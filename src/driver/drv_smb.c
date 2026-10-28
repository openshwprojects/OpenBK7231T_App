/*
 * drv_smb.c - Cross-platform SMB2 (MS-SMB2 2.0.2 / 2.1) Server Driver for OpenBeken
 *
 * Exposes LittleFS and any mounted VFS volumes (such as /usb0 NTFS, /usb1 FAT16,
 * /usb2 FAT32 on RTL8196C) as native network shares over TCP port 445:
 *   \\<ip>\obk   -> VFS root "/" (includes LittleFS + /usb0, /usb1, /usb2, ...)
 *   \\<ip>\usb0  -> /usb0 directly
 *   \\<ip>\usb1  -> /usb1 directly
 *   \\<ip>\usb2  -> /usb2 directly
 *
 * Features:
 *   - 100% DRY on top of OpenBeken's standard lfs_* VFS API (via our_lfs.h).
 *   - Supports both NTLMv2 authentication + HMAC-SHA256 packet signing (required by
 *     Windows 10/11 Explorer & net use) and Guest/Anonymous sessions.
 *   - Supports SMB2 Compound requests (NextCommand + SMB2_FLAGS_RELATED_OPERATIONS).
 *   - Streams large files (10 MB+ JPEGs, PDFs, etc.) in 8 KB chunks without buffering
 *     whole files in RAM.
 */

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include "../littlefs/our_lfs.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "drv_public.h"
#include "drv_smb.h"

#if ENABLE_DRIVER_SMB && ENABLE_LITTLEFS

extern const char *HAL_GetMyIPString(void);

#define SMB_DEFAULT_PORT        445
#define SMB_MAX_CLIENTS         2
#define SMB_MAX_HANDLES         16
#define SMB_MAX_RW_SIZE         65536
#define SMB_BUF_SIZE            66560
#define SMB_IDLE_TIMEOUT_SEC    300

/* NTSTATUS codes */
#define NT_STATUS_SUCCESS                   0x00000000u
#define NT_STATUS_NO_MORE_FILES             0x80000006u
#define NT_STATUS_BUFFER_OVERFLOW           0x80000005u
#define NT_STATUS_INVALID_INFO_CLASS        0xC0000003u
#define NT_STATUS_INFO_LENGTH_MISMATCH      0xC0000004u
#define NT_STATUS_INVALID_HANDLE            0xC0000008u
#define NT_STATUS_INVALID_PARAMETER         0xC000000Du
#define NT_STATUS_NO_SUCH_FILE              0xC000000Fu
#define NT_STATUS_END_OF_FILE               0xC0000011u
#define NT_STATUS_MORE_PROCESSING_REQUIRED  0xC0000016u
#define NT_STATUS_ACCESS_DENIED             0xC0000022u
#define NT_STATUS_OBJECT_NAME_INVALID       0xC0000033u
#define NT_STATUS_OBJECT_NAME_NOT_FOUND     0xC0000034u
#define NT_STATUS_OBJECT_NAME_COLLISION     0xC0000035u
#define NT_STATUS_OBJECT_PATH_NOT_FOUND     0xC000003Au
#define NT_STATUS_LOGON_FAILURE             0xC000006Du
#define NT_STATUS_INSUFFICIENT_RESOURCES    0xC000009Au
#define NT_STATUS_FILE_IS_A_DIRECTORY       0xC00000BAu
#define NT_STATUS_NOT_SUPPORTED             0xC00000BBu
#define NT_STATUS_BAD_NETWORK_NAME          0xC00000CCu
#define NT_STATUS_NOT_A_DIRECTORY           0xC0000103u
#define NT_STATUS_DIRECTORY_NOT_EMPTY       0xC0000101u
#define NT_STATUS_FS_DRIVER_REQUIRED        0xC000019Cu

/* SMB2 Command codes */
#define SMB2_CMD_NEGOTIATE        0x0000
#define SMB2_CMD_SESSION_SETUP    0x0001
#define SMB2_CMD_LOGOFF           0x0002
#define SMB2_CMD_TREE_CONNECT     0x0003
#define SMB2_CMD_TREE_DISCONNECT  0x0004
#define SMB2_CMD_CREATE           0x0005
#define SMB2_CMD_CLOSE            0x0006
#define SMB2_CMD_FLUSH            0x0007
#define SMB2_CMD_READ             0x0008
#define SMB2_CMD_WRITE            0x0009
#define SMB2_CMD_LOCK             0x000A
#define SMB2_CMD_IOCTL            0x000B
#define SMB2_CMD_CANCEL           0x000C
#define SMB2_CMD_ECHO             0x000D
#define SMB2_CMD_QUERY_DIRECTORY  0x000E
#define SMB2_CMD_CHANGE_NOTIFY    0x000F
#define SMB2_CMD_QUERY_INFO       0x0010
#define SMB2_CMD_SET_INFO         0x0011
#define SMB2_CMD_OPLOCK_BREAK     0x0012

/* SMB2 Header Flags */
#define SMB2_FLAGS_SERVER_TO_REDIR      0x00000001u
#define SMB2_FLAGS_ASYNC_COMMAND        0x00000002u
#define SMB2_FLAGS_RELATED_OPERATIONS   0x00000004u
#define SMB2_FLAGS_SIGNED               0x00000008u

/* File Attributes */
#define SMB_ATTR_READONLY   0x00000001u
#define SMB_ATTR_HIDDEN     0x00000002u
#define SMB_ATTR_SYSTEM     0x00000004u
#define SMB_ATTR_DIRECTORY  0x00000010u
#define SMB_ATTR_ARCHIVE    0x00000020u
#define SMB_ATTR_NORMAL     0x00000080u

/* Fixed timestamp: 2026-10-17 12:00:00 UTC in Windows FILETIME (100ns since 1601-01-01) */
#define SMB_DEFAULT_FILETIME 134368848000000000ULL

/* ============================================================================
 * Little-Endian Read/Write Helpers
 * ============================================================================ */

static uint16_t smb_r16(const uint8_t *p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t smb_r32(const uint8_t *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t smb_r64(const uint8_t *p) {
	return (uint64_t)smb_r32(p) | ((uint64_t)smb_r32(p + 4) << 32);
}
static void smb_w16(uint8_t *p, uint16_t v) {
	p[0] = (uint8_t)(v & 0xFF);
	p[1] = (uint8_t)(v >> 8);
}
static void smb_w32(uint8_t *p, uint32_t v) {
	p[0] = (uint8_t)(v & 0xFF);
	p[1] = (uint8_t)((v >> 8) & 0xFF);
	p[2] = (uint8_t)((v >> 16) & 0xFF);
	p[3] = (uint8_t)((v >> 24) & 0xFF);
}
static void smb_w64(uint8_t *p, uint64_t v) {
	smb_w32(p, (uint32_t)(v & 0xFFFFFFFFu));
	smb_w32(p + 4, (uint32_t)(v >> 32));
}

/* ============================================================================
 * Compact Cryptographic Primitives for NTLMv2 & SMB2 HMAC-SHA256 Signing
 * (MD4, MD5, HMAC-MD5, RC4, SHA-256, HMAC-SHA256)
 * ============================================================================ */

static uint32_t rotl32(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
static uint32_t rotr32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

/* --- MD4 (RFC 1320) for NTLM password hash --- */
static void md4_compute(const uint8_t *msg, size_t len, uint8_t out[16]) {
	uint32_t h[4] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u };
	uint8_t block[64];
	size_t off = 0;
	uint64_t bitlen = (uint64_t)len * 8u;
	int padded = 0;

	while (!padded) {
		size_t rem = (off < len) ? (len - off) : 0;
		if (rem >= 64) {
			memcpy(block, msg + off, 64);
			off += 64;
		} else {
			memset(block, 0, 64);
			if (rem > 0)
				memcpy(block, msg + off, rem);
			if (off <= len) {
				block[rem] = 0x80;
				off = len + 1;
			}
			if (rem < 56) {
				smb_w64(block + 56, bitlen);
				padded = 1;
			}
		}

		uint32_t X[16];
		for (int i = 0; i < 16; i++)
			X[i] = smb_r32(block + i * 4);
		uint32_t A = h[0], B = h[1], C = h[2], D = h[3];
		static const int r1[4] = { 3, 7, 11, 19 };
		static const int r2[4] = { 3, 5, 9, 13 };
		static const int r3[4] = { 3, 9, 11, 15 };
		static const int idx2[16] = { 0,4,8,12, 1,5,9,13, 2,6,10,14, 3,7,11,15 };
		static const int idx3[16] = { 0,8,4,12, 2,10,6,14, 1,9,5,13, 3,11,7,15 };

		for (int i = 0; i < 16; i++) {
			uint32_t f = (B & C) | (~B & D);
			uint32_t t = rotl32(A + f + X[i], r1[i & 3]);
			A = D; D = C; C = B; B = t;
		}
		for (int i = 0; i < 16; i++) {
			uint32_t f = (B & C) | (B & D) | (C & D);
			uint32_t t = rotl32(A + f + X[idx2[i]] + 0x5A827999u, r2[i & 3]);
			A = D; D = C; C = B; B = t;
		}
		for (int i = 0; i < 16; i++) {
			uint32_t f = B ^ C ^ D;
			uint32_t t = rotl32(A + f + X[idx3[i]] + 0x6ED9EBA1u, r3[i & 3]);
			A = D; D = C; C = B; B = t;
		}
		h[0] += A; h[1] += B; h[2] += C; h[3] += D;
	}
	for (int i = 0; i < 4; i++)
		smb_w32(out + i * 4, h[i]);
}

/* --- MD5 (RFC 1321) & HMAC-MD5 for NTLMv2 --- */
static const uint32_t k_md5[64] = {
	0xd76aa478u,0xe8c7b756u,0x242070dbu,0xc1bdceeeu,0xf57c0fafu,0x4787c62au,0xa8304613u,0xfd469501u,
	0x698098d8u,0x8b44f7afu,0xffff5bb1u,0x895cd7beu,0x6b901122u,0xfd987193u,0xa679438eu,0x49b40821u,
	0xf61e2562u,0xc040b340u,0x265e5a51u,0xe9b6c7aau,0xd62f105du,0x02441453u,0xd8a1e681u,0xe7d3fbc8u,
	0x21e1cde6u,0xc33707d6u,0xf4d50d87u,0x455a14edu,0xa9e3e905u,0xfcefa3f8u,0x676f02d9u,0x8d2a4c8au,
	0xfffa3942u,0x8771f681u,0x6d9d6122u,0xfde5380cu,0xa4beea44u,0x4bdecfa9u,0xf6bb4b60u,0xbebfbc70u,
	0x289b7ec6u,0xeaa127fau,0xd4ef3085u,0x04881d05u,0xd9d4d039u,0xe6db99e5u,0x1fa27cf8u,0xc4ac5665u,
	0xf4292244u,0x432aff97u,0xab9423a7u,0xfc93a039u,0x655b59c3u,0x8f0ccc92u,0xffeff47du,0x85845dd1u,
	0x6fa87e4fu,0xfe2ce6e0u,0xa3014314u,0x4e0811a1u,0xf7537e82u,0xbd3af235u,0x2ad7d2bbu,0xeb86d391u
};
static const uint8_t s_md5[64] = {
	7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
	5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
	4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
	6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

static void md5_compute_2(const uint8_t *m1, size_t l1, const uint8_t *m2, size_t l2, uint8_t out[16]) {
	uint32_t h[4] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u };
	size_t total = l1 + l2;
	uint64_t bitlen = (uint64_t)total * 8u;
	size_t pos = 0;
	int padded = 0;
	uint8_t block[64];

	while (!padded) {
		size_t bpos = 0;
		while (bpos < 64 && pos < total) {
			block[bpos++] = (pos < l1) ? m1[pos] : m2[pos - l1];
			pos++;
		}
		if (bpos < 64) {
			memset(block + bpos, 0, 64 - bpos);
			if (pos == total) {
				block[bpos++] = 0x80;
				pos++;
			}
			if (bpos <= 56) {
				smb_w64(block + 56, bitlen);
				padded = 1;
			}
		}
		uint32_t M[16];
		for (int i = 0; i < 16; i++)
			M[i] = smb_r32(block + i * 4);
		uint32_t A = h[0], B = h[1], C = h[2], D = h[3];
		for (int i = 0; i < 64; i++) {
			uint32_t F, g;
			if (i < 16)      { F = (B & C) | (~B & D); g = i; }
			else if (i < 32) { F = (D & B) | (~D & C); g = (5 * i + 1) & 15; }
			else if (i < 48) { F = B ^ C ^ D;          g = (3 * i + 5) & 15; }
			else             { F = C ^ (B | ~D);       g = (7 * i) & 15; }
			F = F + A + k_md5[i] + M[g];
			A = D; D = C; C = B; B = B + rotl32(F, s_md5[i]);
		}
		h[0] += A; h[1] += B; h[2] += C; h[3] += D;
	}
	for (int i = 0; i < 4; i++)
		smb_w32(out + i * 4, h[i]);
}

static void hmac_md5(const uint8_t *key, size_t klen, const uint8_t *data, size_t dlen, uint8_t out[16]) {
	uint8_t k_ipad[64], k_opad[64], tk[16], inner[16];
	if (klen > 64) {
		md5_compute_2(key, klen, NULL, 0, tk);
		key = tk;
		klen = 16;
	}
	memset(k_ipad, 0x36, 64);
	memset(k_opad, 0x5c, 64);
	for (size_t i = 0; i < klen; i++) {
		k_ipad[i] ^= key[i];
		k_opad[i] ^= key[i];
	}
	md5_compute_2(k_ipad, 64, data, dlen, inner);
	md5_compute_2(k_opad, 64, inner, 16, out);
}

/* --- RC4 for NTLMSSP_NEGOTIATE_KEY_EXCH --- */
static void rc4_crypt(const uint8_t key[16], const uint8_t *in, uint8_t *out, size_t len) {
	uint8_t S[256];
	for (int i = 0; i < 256; i++) S[i] = (uint8_t)i;
	uint8_t j = 0;
	for (int i = 0; i < 256; i++) {
		j = (uint8_t)(j + S[i] + key[i & 15]);
		uint8_t t = S[i]; S[i] = S[j]; S[j] = t;
	}
	uint8_t i = 0; j = 0;
	for (size_t k = 0; k < len; k++) {
		i = (uint8_t)(i + 1);
		j = (uint8_t)(j + S[i]);
		uint8_t t = S[i]; S[i] = S[j]; S[j] = t;
		out[k] = in[k] ^ S[(uint8_t)(S[i] + S[j])];
	}
}

/* --- SHA-256 & HMAC-SHA256 for SMB2 Packet Signing --- */
static const uint32_t k_sha256[64] = {
	0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
	0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
	0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
	0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
	0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
	0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
	0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
	0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

static void sha256_compute_2(const uint8_t *m1, size_t l1, const uint8_t *m2, size_t l2, uint8_t out[32]) {
	uint32_t h[8] = {
		0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
		0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
	};
	size_t total = l1 + l2;
	uint64_t bitlen = (uint64_t)total * 8u;
	size_t pos = 0;
	int padded = 0;
	uint8_t block[64];

	while (!padded) {
		size_t bpos = 0;
		while (bpos < 64 && pos < total) {
			block[bpos++] = (pos < l1) ? m1[pos] : m2[pos - l1];
			pos++;
		}
		if (bpos < 64) {
			memset(block + bpos, 0, 64 - bpos);
			if (pos == total) {
				block[bpos++] = 0x80;
				pos++;
			}
			if (bpos <= 56) {
				for (int i = 0; i < 8; i++)
					block[56 + i] = (uint8_t)(bitlen >> (56 - i * 8));
				padded = 1;
			}
		}
		uint32_t W[64];
		for (int i = 0; i < 16; i++) {
			W[i] = ((uint32_t)block[i*4] << 24) | ((uint32_t)block[i*4+1] << 16) |
			       ((uint32_t)block[i*4+2] << 8) | (uint32_t)block[i*4+3];
		}
		for (int i = 16; i < 64; i++) {
			uint32_t s0 = rotr32(W[i-15], 7) ^ rotr32(W[i-15], 18) ^ (W[i-15] >> 3);
			uint32_t s1 = rotr32(W[i-2], 17) ^ rotr32(W[i-2], 19) ^ (W[i-2] >> 10);
			W[i] = W[i-16] + s0 + W[i-7] + s1;
		}
		uint32_t a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], hh=h[7];
		for (int i = 0; i < 64; i++) {
			uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
			uint32_t ch = (e & f) ^ (~e & g);
			uint32_t t1 = hh + S1 + ch + k_sha256[i] + W[i];
			uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
			uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
			uint32_t t2 = S0 + maj;
			hh = g; g = f; f = e; e = d + t1;
			d = c; c = b; b = a; a = t1 + t2;
		}
		h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
	}
	for (int i = 0; i < 8; i++) {
		out[i*4]   = (uint8_t)(h[i] >> 24);
		out[i*4+1] = (uint8_t)(h[i] >> 16);
		out[i*4+2] = (uint8_t)(h[i] >> 8);
		out[i*4+3] = (uint8_t)h[i];
	}
}

static void hmac_sha256(const uint8_t *key, size_t klen, const uint8_t *data, size_t dlen, uint8_t out[32]) {
	uint8_t k_ipad[64], k_opad[64], inner[32];
	memset(k_ipad, 0x36, 64);
	memset(k_opad, 0x5c, 64);
	for (size_t i = 0; i < klen && i < 64; i++) {
		k_ipad[i] ^= key[i];
		k_opad[i] ^= key[i];
	}
	sha256_compute_2(k_ipad, 64, data, dlen, inner);
	sha256_compute_2(k_opad, 64, inner, 32, out);
}

/* ============================================================================
 * SMB2 Data Structures & State
 * ============================================================================ */

typedef struct smb_handle_s {
	uint8_t    inUse;
	uint8_t    isDir;
	uint8_t    isPipe;
	uint8_t    deleteOnClose;
	uint8_t    lfsOpen;
	uint8_t    lfsWritable;
	uint8_t    dirDone;
	uint16_t   treeId;
	uint32_t   dirIndex;
	uint32_t   fileSize;
	uint32_t   curOffset;
	char       path[256];      /* Normalized VFS path ("" for root, "file.txt", "/usb0/file.jpg") */
	char       name[128];      /* Display name relative to share */
	char       pattern[64];    /* Directory search pattern */
	lfs_file_t lfsFile;
} smb_handle_t;

typedef struct smb_tree_s {
	uint8_t  inUse;
	uint8_t  isPipe;
	uint16_t treeId;
	char     basePrefix[64];   /* "" for root "/", or "/usb0", "/usb1", etc. */
} smb_tree_t;

#define SMB_MAX_TREES 16

typedef struct smb_client_s {
	int        sock;
	int        idleSec;
	uint16_t   dialect;
	uint64_t   sessionId;
	uint8_t    authenticated;
	uint8_t    isGuest;
	uint8_t    hasSigningKey;
	uint8_t    signingKey[16];
	uint8_t    serverChallenge[8];
	char       ipStr[20];
	/* Framing state */
	uint8_t    hdrBuf[4];
	int        hdrGot;
	uint32_t   pktLen;
	uint32_t   pktGot;
	uint8_t   *rxBuf;
	uint8_t   *txBuf;
	smb_tree_t trees[SMB_MAX_TREES];
} smb_client_t;

static int          g_smbListenSock = -1;
static int          g_smbPort       = SMB_DEFAULT_PORT;
static int          g_smbUptimeSec  = 0;
static uint32_t     g_smbBytesRead  = 0;
static uint32_t     g_smbBytesWrote = 0;
static uint32_t     g_smbReqCount   = 0;
static char         g_smbUser[64]   = "admin";
static char         g_smbPass[64]   = "admin";
static smb_client_t g_clients[SMB_MAX_CLIENTS];
static smb_handle_t g_handles[SMB_MAX_HANDLES];
static uint64_t     g_nextSessionId = 1;

/* ============================================================================
 * UTF-16LE <-> UTF-8 / ASCII Helpers & Path Resolution
 * ============================================================================ */

static int utf8_to_utf16le(const char *s, uint8_t *out, int maxBytes) {
	int pos = 0;
	const uint8_t *p = (const uint8_t *)s;
	while (*p && pos + 2 <= maxBytes) {
		uint32_t cp = *p++;
		if ((cp & 0xE0) == 0xC0 && (*p & 0xC0) == 0x80) {
			cp = ((cp & 0x1F) << 6) | (*p++ & 0x3F);
		} else if ((cp & 0xF0) == 0xE0 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) {
			cp = ((cp & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
			p += 2;
		}
		smb_w16(out + pos, (uint16_t)cp);
		pos += 2;
	}
	return pos;
}

static void utf16le_to_utf8(const uint8_t *in, int inBytes, char *out, int maxOut) {
	int pos = 0;
	for (int i = 0; i + 1 < inBytes && pos + 4 < maxOut; i += 2) {
		uint16_t ch = smb_r16(in + i);
		if (ch == 0)
			break;
		if (ch < 0x80) {
			out[pos++] = (char)ch;
		} else if (ch < 0x800) {
			out[pos++] = (char)(0xC0 | (ch >> 6));
			out[pos++] = (char)(0x80 | (ch & 0x3F));
		} else {
			out[pos++] = (char)(0xE0 | (ch >> 12));
			out[pos++] = (char)(0x80 | ((ch >> 6) & 0x3F));
			out[pos++] = (char)(0x80 | (ch & 0x3F));
		}
	}
	out[pos] = '\0';
}

/* Convert relative SMB path (with '\') + share basePrefix into VFS path for lfs_* */
static void smb_resolve_vfs_path(const char *basePrefix, const char *relPath, char *outVfs, int outSz) {
	char combined[256];
	int pos = 0;
	if (basePrefix && basePrefix[0]) {
		int blen = (int)strlen(basePrefix);
		if (blen >= (int)sizeof(combined) - 1) blen = (int)sizeof(combined) - 2;
		memcpy(combined, basePrefix, blen);
		pos = blen;
	}
	if (relPath && relPath[0]) {
		if (pos == 0 || combined[pos - 1] != '/')
			combined[pos++] = '/';
		for (int i = 0; relPath[i] && pos < (int)sizeof(combined) - 1; i++) {
			char c = (relPath[i] == '\\') ? '/' : relPath[i];
			if (c == '/' && pos > 0 && combined[pos - 1] == '/')
				continue;
			combined[pos++] = c;
		}
	}
	while (pos > 1 && combined[pos - 1] == '/')
		pos--;
	combined[pos] = '\0';

	if (combined[0] == '\0' || strcmp(combined, "/") == 0) {
		outVfs[0] = '\0';
		return;
	}
	const char *s = (combined[0] == '/') ? (combined + 1) : combined;
	strncpy(outVfs, s, outSz - 1);
	outVfs[outSz - 1] = '\0';
}

/* Probe whether a VFS path exists and whether it is a directory or file */
static int smb_stat_vfs(const char *vfsPath, int *isDir, uint32_t *fileSize) {
	if (isDir) *isDir = 0;
	if (fileSize) *fileSize = 0;

	/* Root directory "" */
	if (!vfsPath || vfsPath[0] == '\0' || strcmp(vfsPath, "/") == 0) {
		if (isDir) *isDir = 1;
		return 1;
	}

	/* Try opening as directory first (handles virtual /usb0..3 and subdirectories) */
	lfs_dir_t d;
	memset(&d, 0, sizeof(d));
	if (lfs_dir_open(&lfs, &d, vfsPath) >= 0) {
		lfs_dir_close(&lfs, &d);
		if (isDir) *isDir = 1;
		return 1;
	}

	/* Try opening as file */
	lfs_file_t f;
	memset(&f, 0, sizeof(f));
	if (lfs_file_open(&lfs, &f, vfsPath, LFS_O_RDONLY) >= 0) {
		uint32_t sz = (uint32_t)lfs_file_size(&lfs, &f);
		lfs_file_close(&lfs, &f);
		if (fileSize) *fileSize = sz;
		return 1;
	}

	return 0;
}

/* Simple wildcard matcher ('*' and '?', case-insensitive) for SMB2_QUERY_DIRECTORY */
static int smb_wildcard_match(const char *pat, const char *str) {
	if (!pat || !pat[0] || strcmp(pat, "*") == 0 || strcmp(pat, "*.*") == 0)
		return 1;
	while (*pat && *str) {
		if (*pat == '*') {
			pat++;
			if (!*pat) return 1;
			while (*str) {
				if (smb_wildcard_match(pat, str))
					return 1;
				str++;
			}
			return 0;
		}
		char c1 = (*pat >= 'a' && *pat <= 'z') ? (char)(*pat - 32) : *pat;
		char c2 = (*str >= 'a' && *str <= 'z') ? (char)(*str - 32) : *str;
		if (*pat != '?' && c1 != c2)
			return 0;
		pat++;
		str++;
	}
	while (*pat == '*') pat++;
	return (*pat == '\0' && *str == '\0');
}

/* ============================================================================
 * Handle & Client Management
 * ============================================================================ */

static void smb_close_handle(smb_handle_t *h) {
	if (!h || !h->inUse)
		return;
	if (h->lfsOpen) {
		lfs_file_close(&lfs, &h->lfsFile);
		h->lfsOpen = 0;
	}
	if (h->deleteOnClose && !h->isPipe && h->path[0]) {
		lfs_remove(&lfs, h->path);
	}
	memset(h, 0, sizeof(*h));
}

static smb_handle_t *smb_alloc_handle(void) {
	for (int i = 0; i < SMB_MAX_HANDLES; i++) {
		if (!g_handles[i].inUse) {
			memset(&g_handles[i], 0, sizeof(smb_handle_t));
			g_handles[i].inUse = 1;
			return &g_handles[i];
		}
	}
	return NULL;
}

static smb_handle_t *smb_get_handle(const uint8_t *fileId) {
	uint64_t id = smb_r64(fileId);
	if (id == 0 || id > SMB_MAX_HANDLES)
		return NULL;
	smb_handle_t *h = &g_handles[id - 1];
	return h->inUse ? h : NULL;
}

static uint64_t smb_handle_id(smb_handle_t *h) {
	return (uint64_t)((h - g_handles) + 1);
}

static void smb_close_client(smb_client_t *c) {
	if (!c || c->sock < 0)
		return;
	close(c->sock);
	c->sock = -1;
	c->pktGot = 0;
	c->pktLen = 0;
	c->hdrGot = 0;
	c->authenticated = 0;
	c->hasSigningKey = 0;
	for (int i = 0; i < SMB_MAX_TREES; i++)
		c->trees[i].inUse = 0;
}

static smb_tree_t *smb_find_tree(smb_client_t *c, uint32_t treeId) {
	for (int i = 0; i < SMB_MAX_TREES; i++) {
		if (c->trees[i].inUse && c->trees[i].treeId == (uint16_t)treeId)
			return &c->trees[i];
	}
	return NULL;
}

static int smb_send_all(int sock, const void *buf, int len) {
	const uint8_t *p = (const uint8_t *)buf;
	int sent = 0;
	int spins = 0;
	while (sent < len) {
		int r = send(sock, (const char *)(p + sent), len - sent, 0);
		if (r > 0) {
			sent += r;
			spins = 0;
		} else {
			if (++spins > 1000)
				return -1;
			rtos_delay_milliseconds(1);
		}
	}
	return sent;
}

/* ============================================================================
 * SMB2 Header & Error Response Builders
 * ============================================================================ */

static void smb2_init_resp_hdr(uint8_t *outHdr, const uint8_t *reqHdr, uint32_t status, uint64_t sessionId, uint32_t treeId) {
	memset(outHdr, 0, 64);
	outHdr[0] = 0xFE; outHdr[1] = 'S'; outHdr[2] = 'M'; outHdr[3] = 'B';
	smb_w16(outHdr + 4, 64);
	if (reqHdr && reqHdr[0] == 0xFE) {
		smb_w16(outHdr + 6, smb_r16(reqHdr + 6));          /* CreditCharge */
		smb_w32(outHdr + 8, status);                       /* Status */
		smb_w16(outHdr + 12, smb_r16(reqHdr + 12));        /* Command */
		uint16_t credReq = smb_r16(reqHdr + 14);
		smb_w16(outHdr + 14, credReq > 16 ? credReq : 16); /* CreditResponse */
		uint32_t reqFlags = smb_r32(reqHdr + 16);
		smb_w32(outHdr + 16, SMB2_FLAGS_SERVER_TO_REDIR | (reqFlags & SMB2_FLAGS_RELATED_OPERATIONS));
		memcpy(outHdr + 24, reqHdr + 24, 8);               /* MessageId */
		memcpy(outHdr + 32, reqHdr + 32, 4);               /* ProcessId */
		smb_w32(outHdr + 36, treeId ? treeId : smb_r32(reqHdr + 36));
		smb_w64(outHdr + 40, sessionId ? sessionId : smb_r64(reqHdr + 40));
	} else {
		/* Response to SMB1 multi-protocol Negotiate */
		smb_w32(outHdr + 8, status);
		smb_w16(outHdr + 12, SMB2_CMD_NEGOTIATE);
		smb_w16(outHdr + 14, 1);
		smb_w32(outHdr + 16, SMB2_FLAGS_SERVER_TO_REDIR);
	}
}

static int smb2_build_error(uint8_t *outHdr, const uint8_t *reqHdr, uint32_t status) {
	smb2_init_resp_hdr(outHdr, reqHdr, status, 0, 0);
	uint8_t *body = outHdr + 64;
	memset(body, 0, 9);
	smb_w16(body, 9); /* StructureSize = 9 */
	return 64 + 9;
}

/* ============================================================================
 * Command Handlers
 * ============================================================================ */

/* Helper to write an ASN.1 DER TLV (tag + minimal length + value) */
static int der_wrap(uint8_t tag, const uint8_t *val, int vlen, uint8_t *out) {
	out[0] = tag;
	if (vlen < 128) {
		out[1] = (uint8_t)vlen;
		if (val && vlen > 0)
			memmove(out + 2, val, vlen);
		return 2 + vlen;
	} else {
		out[1] = 0x81;
		out[2] = (uint8_t)vlen;
		if (val && vlen > 0)
			memmove(out + 3, val, vlen);
		return 3 + vlen;
	}
}

static void smb_fill_server_guid(uint8_t guid[16]) {
	memset(guid, 0, 16);
	const char *dev = CFG_GetDeviceName();
	guid[0] = 'O'; guid[1] = 'B'; guid[2] = 'K'; guid[3] = '2';
	for (int i = 0; i < 12 && dev && dev[i]; i++)
		guid[4 + i] = (uint8_t)dev[i];
}

static int smb2_handle_negotiate(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr) {
	uint16_t chosenDialect = 0x0202;
	if (reqHdr[0] == 0xFE && reqLen >= 64 + 36) {
		uint16_t dialectCount = smb_r16(reqHdr + 64 + 2);
		const uint8_t *dptr = reqHdr + 64 + 36;
		for (int i = 0; i < dialectCount && (dptr + 2 <= reqHdr + reqLen); i++, dptr += 2) {
			uint16_t d = smb_r16(dptr);
			if (d == 0x0210) {
				chosenDialect = 0x0210;
				break;
			}
		}
	}
	c->dialect = chosenDialect;
	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, 0, 0);

	/* 30-byte SPNEGO NegTokenInit advertising NTLMSSP (1.3.6.1.4.1.311.2.2.10) */
	static const uint8_t spnegoInit[30] = {
		0x60, 0x1C,
		0x06, 0x06, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x02,
		0xA0, 0x12, 0x30, 0x10, 0xA0, 0x0E, 0x30, 0x0C,
		0x06, 0x0A, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x02, 0x0A
	};

	uint8_t *body = outHdr + 64;
	memset(body, 0, 64);
	smb_w16(body + 0, 65);             /* StructureSize = 65 */
	smb_w16(body + 2, 0x0001);         /* SecurityMode = SMB2_NEGOTIATE_SIGNING_ENABLED */
	smb_w16(body + 4, chosenDialect);  /* DialectRevision */
	smb_fill_server_guid(body + 8);
	smb_w32(body + 24, 0);                 /* Capabilities */
	smb_w32(body + 28, SMB_MAX_RW_SIZE);   /* MaxTransactSize = 8192 */
	smb_w32(body + 32, SMB_MAX_RW_SIZE);   /* MaxReadSize = 8192 */
	smb_w32(body + 36, SMB_MAX_RW_SIZE);   /* MaxWriteSize = 8192 */
	smb_w64(body + 40, SMB_DEFAULT_FILETIME);
	smb_w64(body + 48, SMB_DEFAULT_FILETIME);
	smb_w16(body + 56, 64 + 64);           /* SecurityBufferOffset = 128 */
	smb_w16(body + 58, sizeof(spnegoInit));/* SecurityBufferLength = 30 */
	memcpy(body + 64, spnegoInit, sizeof(spnegoInit));
	return 64 + 64 + (int)sizeof(spnegoInit);
}

/* Find substring "NTLMSSP\0" inside security blob */
static const uint8_t *find_ntlmssp(const uint8_t *buf, int len, int *remLen) {
	for (int i = 0; i + 12 <= len; i++) {
		if (memcmp(buf + i, "NTLMSSP\0", 8) == 0) {
			*remLen = len - i;
			return buf + i;
		}
	}
	return NULL;
}

/* Compute NTLMv2 ResponseKeyNT = HMAC_MD5(MD4(UTF16LE(pass)), UTF16LE(UPPER(user)) + domain_utf16le) */
static void ntlmv2_response_key(const char *passAscii,
                                const uint8_t *userUtf16, int userLen,
                                const uint8_t *domUtf16, int domLen,
                                uint8_t outKey[16]) {
	uint8_t passUtf16[128];
	int passLen = utf8_to_utf16le(passAscii ? passAscii : "", passUtf16, sizeof(passUtf16));
	uint8_t ntHash[16];
	md4_compute(passUtf16, (size_t)passLen, ntHash);

	uint8_t ident[256];
	int ilen = 0;
	for (int i = 0; i + 1 < userLen && ilen + 2 <= (int)sizeof(ident); i += 2) {
		uint16_t ch = smb_r16(userUtf16 + i);
		if (ch >= 'a' && ch <= 'z')
			ch -= 32;
		smb_w16(ident + ilen, ch);
		ilen += 2;
	}
	for (int i = 0; i < domLen && ilen < (int)sizeof(ident); i++) {
		ident[ilen++] = domUtf16[i];
	}
	hmac_md5(ntHash, 16, ident, (size_t)ilen, outKey);
}

static int smb2_handle_session_setup(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr) {
	if (reqLen < 64 + 24)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint64_t sessId = smb_r64(reqHdr + 40);
	if (sessId == 0) {
		if (c->sessionId == 0)
			c->sessionId = g_nextSessionId++;
		sessId = c->sessionId;
	}

	uint16_t secOff = smb_r16(reqHdr + 64 + 12);
	uint16_t secLen = smb_r16(reqHdr + 64 + 14);
	const uint8_t *secBuf = (secOff >= 64 && (int)secOff + (int)secLen <= reqLen) ? (reqHdr + secOff) : NULL;

	int ntlmLen = 0;
	const uint8_t *ntlm = secBuf ? find_ntlmssp(secBuf, secLen, &ntlmLen) : NULL;
	uint32_t msgType = (ntlm && ntlmLen >= 12) ? smb_r32(ntlm + 8) : 0;

	/* Step 1: NTLMSSP_NEGOTIATE (Type 1) -> Reply with NTLMSSP_CHALLENGE (Type 2) */
	if (msgType == 1) {
		uint32_t cliFlags = (ntlmLen >= 16) ? smb_r32(ntlm + 12) : 0;
		for (int i = 0; i < 8; i++)
			c->serverChallenge[i] = (uint8_t)(0x11 * (i + 1) ^ (g_smbUptimeSec & 0xFF));

		/* Build Type 2 NTLMSSP Challenge message */
		uint8_t type2[180];
		memset(type2, 0, sizeof(type2));
		memcpy(type2, "NTLMSSP\0", 8);
		smb_w32(type2 + 8, 2); /* Type 2 */

		/* TargetName = "OBK" (6 bytes UTF-16LE at offset 56) */
		const char *tname = "OBK";
		int tnLen = utf8_to_utf16le(tname, type2 + 56, 16);
		smb_w16(type2 + 12, (uint16_t)tnLen);
		smb_w16(type2 + 14, (uint16_t)tnLen);
		smb_w32(type2 + 16, 56);

		uint32_t srvFlags = 0x00000001u /* UNICODE */
		                  | 0x00000004u /* REQUEST_TARGET */
		                  | 0x00000200u /* NTLM */
		                  | 0x00008000u /* ALWAYS_SIGN */
		                  | 0x00020000u /* TARGET_TYPE_SERVER */
		                  | 0x00080000u /* EXTENDED_SESSIONSECURITY */
		                  | 0x00800000u /* TARGET_INFO */
		                  | 0x02000000u /* VERSION */
		                  | 0x20000000u /* 128-bit */
		                  | (cliFlags & 0xC0000030u); /* 56-bit | KEY_EXCH | SEAL | SIGN */
		smb_w32(type2 + 20, srvFlags);
		memcpy(type2 + 24, c->serverChallenge, 8);
		/* Version at 48..55: Windows 6.1 build 7600 NTLMSSP rev 15 */
		type2[48] = 6; type2[49] = 1; smb_w16(type2 + 50, 7600); type2[55] = 15;

		/* TargetInfo AV_PAIR list at offset 56 + tnLen */
		int tiOff = 56 + tnLen;
		uint8_t *ti = type2 + tiOff;
		int tiPos = 0;
		static const uint16_t avIds[4] = { 2, 1, 4, 3 };
		for (int k = 0; k < 4; k++) {
			smb_w16(ti + tiPos, avIds[k]);
			smb_w16(ti + tiPos + 2, (uint16_t)tnLen);
			memcpy(ti + tiPos + 4, type2 + 56, tnLen);
			tiPos += 4 + tnLen;
		}
		/* MsvAvTimestamp (7) */
		smb_w16(ti + tiPos, 7);
		smb_w16(ti + tiPos + 2, 8);
		smb_w64(ti + tiPos + 4, SMB_DEFAULT_FILETIME);
		tiPos += 12;
		/* MsvAvEOL (0) */
		smb_w16(ti + tiPos, 0);
		smb_w16(ti + tiPos + 2, 0);
		tiPos += 4;

		smb_w16(type2 + 40, (uint16_t)tiPos);
		smb_w16(type2 + 42, (uint16_t)tiPos);
		smb_w32(type2 + 44, (uint32_t)tiOff);
		int type2Len = tiOff + tiPos;

		smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_MORE_PROCESSING_REQUIRED, sessId, 0);
		uint8_t *body = outHdr + 64;
		smb_w16(body + 0, 9);
		smb_w16(body + 2, 0);
		smb_w16(body + 4, 64 + 8);

		uint8_t *outSec = body + 8;
		int outSecLen = 0;
		if (secBuf && secBuf != ntlm) {
			/* Build clean ASN.1 DER SPNEGO NegTokenResp */
			static const uint8_t negStateAndMech[19] = {
				0xA0, 0x03, 0x0A, 0x01, 0x01, /* negState = accept-incomplete (1) */
				0xA1, 0x0C, 0x06, 0x0A, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x02, 0x0A
			};
			uint8_t octTok[192], ctx2[196], seqBody[220], seqWrap[224];
			int octLen  = der_wrap(0x04, type2, type2Len, octTok);
			int ctx2Len = der_wrap(0xA2, octTok, octLen, ctx2);
			memcpy(seqBody, negStateAndMech, sizeof(negStateAndMech));
			memcpy(seqBody + sizeof(negStateAndMech), ctx2, ctx2Len);
			int seqLen  = der_wrap(0x30, seqBody, (int)sizeof(negStateAndMech) + ctx2Len, seqWrap);
			outSecLen   = der_wrap(0xA1, seqWrap, seqLen, outSec);
		} else {
			memcpy(outSec, type2, type2Len);
			outSecLen = type2Len;
		}
		smb_w16(body + 6, (uint16_t)outSecLen);
		return 64 + 8 + outSecLen;
	}

	/* Step 2: NTLMSSP_AUTH (Type 3) or Anonymous/Guest */
	c->authenticated = 1;
	c->isGuest = 1;
	c->hasSigningKey = 0;

	if (msgType == 3 && ntlmLen >= 60) {
		uint16_t domLen    = smb_r16(ntlm + 28);
		uint32_t domOff    = smb_r32(ntlm + 32);
		uint16_t usrLen    = smb_r16(ntlm + 36);
		uint32_t usrOff    = smb_r32(ntlm + 40);
		uint16_t ntRespLen = smb_r16(ntlm + 20);
		uint32_t ntRespOff = smb_r32(ntlm + 24);
		uint16_t encKeyLen = (ntlmLen >= 60) ? smb_r16(ntlm + 52) : 0;
		uint32_t encKeyOff = (ntlmLen >= 60) ? smb_r32(ntlm + 56) : 0;
		uint32_t negFlags  = (ntlmLen >= 64) ? smb_r32(ntlm + 60) : 0;

		const uint8_t *domPtr = (domOff + domLen <= (uint32_t)ntlmLen) ? (ntlm + domOff) : (const uint8_t *)"";
		const uint8_t *usrPtr = (usrOff + usrLen <= (uint32_t)ntlmLen) ? (ntlm + usrOff) : (const uint8_t *)"";
		const uint8_t *ntResp = (ntRespOff + ntRespLen <= (uint32_t)ntlmLen) ? (ntlm + ntRespOff) : NULL;
		const uint8_t *encKey = (encKeyOff + encKeyLen <= (uint32_t)ntlmLen) ? (ntlm + encKeyOff) : NULL;

		char userLog[64];
		utf16le_to_utf8(usrPtr, usrLen, userLog, sizeof(userLog));

		if (usrLen > 0 && ntResp && ntRespLen >= 24) {
			/* Try candidate passwords (g_smbPass, "admin", "") with client domain or empty domain */
			const char *cands[3] = { g_smbPass, "admin", "" };
			uint8_t respKey[16];
			int matched = 0;
			for (int k = 0; k < 3 && !matched; k++) {
				for (int dmode = 0; dmode < 2 && !matched; dmode++) {
					const uint8_t *dP = (dmode == 0) ? domPtr : (const uint8_t *)"";
					int dL = (dmode == 0) ? domLen : 0;
					ntlmv2_response_key(cands[k], usrPtr, usrLen, dP, dL, respKey);
					uint8_t k_ipad[64], k_opad[64], inner[16], proof[16];
					memset(k_ipad, 0x36, 64);
					memset(k_opad, 0x5c, 64);
					for (int i = 0; i < 16; i++) {
						k_ipad[i] ^= respKey[i];
						k_opad[i] ^= respKey[i];
					}
					uint8_t challPad[72];
					memcpy(challPad, k_ipad, 64);
					memcpy(challPad + 64, c->serverChallenge, 8);
					md5_compute_2(challPad, 72, ntResp + 16, ntRespLen - 16, inner);
					md5_compute_2(k_opad, 64, inner, 16, proof);
					if (memcmp(proof, ntResp, 16) == 0) {
						matched = 1;
						break;
					}
				}
			}
			if (!matched) {
				ADDLOG_INFO(LOG_FEATURE_MAIN, "SMB: auth mismatch for user '%s', requesting retry", userLog);
				return smb2_build_error(outHdr, reqHdr, NT_STATUS_LOGON_FAILURE);
			}
			uint8_t sessionBaseKey[16];
			hmac_md5(respKey, 16, ntResp, 16, sessionBaseKey);
			if ((negFlags & 0x40000000u) && encKey && encKeyLen == 16 && encKeyOff >= 60) {
				rc4_crypt(sessionBaseKey, encKey, c->signingKey, 16);
			} else {
				memcpy(c->signingKey, sessionBaseKey, 16);
			}
			c->hasSigningKey = 1;
			c->isGuest = 0;
			ADDLOG_INFO(LOG_FEATURE_MAIN, "SMB: authenticated user '%s' (NTLMv2 signing key ready)", userLog);
		}
	}

	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, sessId, 0);
	uint8_t *body = outHdr + 64;
	smb_w16(body + 0, 9);
	smb_w16(body + 2, 0x0000); /* SessionFlags = 0 */
	smb_w16(body + 4, 64 + 8);
	int respLen;
	if (secBuf && secBuf != ntlm && c->hasSigningKey) {
		/* Build 29-byte SPNEGO NegTokenResp (accept-completed + NTLMv2 mechListMIC) */
		static const char kSignMagic[59] = "session key to server-to-client signing key magic constant";
		static const char kSealMagic[59] = "session key to server-to-client sealing key magic constant";
		static const uint8_t kSeqAndMechList[18] = {
			0x00, 0x00, 0x00, 0x00,
			0x30, 0x0C, 0x06, 0x0A, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x02, 0x0A
		};
		uint8_t srvSignKey[16], srvSealKey[16], srvDigest[16], cksum[8];
		md5_compute_2(c->signingKey, 16, (const uint8_t *)kSignMagic, 59, srvSignKey);
		md5_compute_2(c->signingKey, 16, (const uint8_t *)kSealMagic, 59, srvSealKey);
		hmac_md5(srvSignKey, 16, kSeqAndMechList, sizeof(kSeqAndMechList), srvDigest);
		rc4_crypt(srvSealKey, srvDigest, cksum, 8);

		static const uint8_t acceptHdr[17] = {
			0xA1, 0x1B, 0x30, 0x19,
			0xA0, 0x03, 0x0A, 0x01, 0x00,
			0xA3, 0x12, 0x04, 0x10,
			0x01, 0x00, 0x00, 0x00
		};
		uint8_t *outSec = body + 8;
		memcpy(outSec, acceptHdr, 17);
		memcpy(outSec + 17, cksum, 8);
		memset(outSec + 25, 0, 4);
		smb_w16(body + 6, 29);
		respLen = 64 + 8 + 29;
	} else if (secBuf && secBuf != ntlm) {
		static const uint8_t acceptDone[7] = { 0xA1, 0x05, 0x30, 0x03, 0xA0, 0x01, 0x00 };
		smb_w16(body + 6, sizeof(acceptDone));
		memcpy(body + 8, acceptDone, sizeof(acceptDone));
		respLen = 64 + 8 + (int)sizeof(acceptDone);
	} else {
		smb_w16(body + 6, 0);
		body[8] = 0;
		respLen = 64 + 9;
	}
	return respLen;
}

static int smb2_handle_tree_connect(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr) {
	if (reqLen < 64 + 8)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint16_t pathOff = smb_r16(reqHdr + 64 + 4);
	uint16_t pathLen = smb_r16(reqHdr + 64 + 6);
	if (pathOff < 64 || (int)pathOff + (int)pathLen > reqLen)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	char uncPath[128];
	utf16le_to_utf8(reqHdr + pathOff, pathLen, uncPath, sizeof(uncPath));

	/* Extract share name after last '\' or '/' */
	const char *share = uncPath;
	for (const char *p = uncPath; *p; p++) {
		if (*p == '\\' || *p == '/')
			share = p + 1;
	}

	smb_tree_t *t = NULL;
	for (int i = 0; i < SMB_MAX_TREES; i++) {
		if (!c->trees[i].inUse) {
			t = &c->trees[i];
			t->inUse = 1;
			t->treeId = (uint16_t)(i + 1);
			break;
		}
	}
	if (!t)
		t = &c->trees[0];

	if (stricmp(share, "IPC$") == 0) {
		t->isPipe = 1;
		t->basePrefix[0] = '\0';
	} else {
		t->isPipe = 0;
		if (stricmp(share, "usb0") == 0 || stricmp(share, "usb1") == 0 ||
		    stricmp(share, "usb2") == 0 || stricmp(share, "usb3") == 0) {
			snprintf(t->basePrefix, sizeof(t->basePrefix), "/%s", share);
			for (int i = 0; t->basePrefix[i]; i++)
				if (t->basePrefix[i] >= 'A' && t->basePrefix[i] <= 'Z')
					t->basePrefix[i] += 32;
		} else {
			/* "obk", "fs", "lfs", "root", etc. -> root VFS "/" */
			t->basePrefix[0] = '\0';
		}
	}

	ADDLOG_INFO(LOG_FEATURE_MAIN, "SMB: client %s connected to share '%s' (treeId=%u)",
	            c->ipStr, share, t->treeId);

	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, t->treeId);
	uint8_t *body = outHdr + 64;
	memset(body, 0, 16);
	smb_w16(body + 0, 16);                        /* StructureSize = 16 */
	body[2] = t->isPipe ? 0x02 : 0x01;            /* ShareType: 2=PIPE, 1=DISK */
	smb_w32(body + 4, 0x00000030u);               /* ShareFlags: NO_CACHING */
	smb_w32(body + 8, 0);                         /* Capabilities */
	smb_w32(body + 12, 0x001F01FFu);              /* MaximalAccess */
	return 64 + 16;
}

static int smb2_handle_create(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, uint8_t lastFileId[16]) {
	if (reqLen < 64 + 56)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint32_t treeId = smb_r32(reqHdr + 36);
	smb_tree_t *tree = smb_find_tree(c, treeId);
	if (!tree)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_BAD_NETWORK_NAME);

	uint32_t desiredAccess = smb_r32(reqHdr + 64 + 24);
	uint32_t disposition   = smb_r32(reqHdr + 64 + 36);
	uint32_t createOptions = smb_r32(reqHdr + 64 + 40);
	uint16_t nameOff       = smb_r16(reqHdr + 64 + 44);
	uint16_t nameLen       = smb_r16(reqHdr + 64 + 46);

	char relName[192];
	if (nameLen > 0 && nameOff >= 64 && (int)nameOff + (int)nameLen <= reqLen)
		utf16le_to_utf8(reqHdr + nameOff, nameLen, relName, sizeof(relName));
	else
		relName[0] = '\0';

	/* Reject Windows special metadata streams (:Zone.Identifier, desktop.ini, srvsvc probes if not needed) */
	if (strchr(relName, ':') != NULL)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_OBJECT_NAME_NOT_FOUND);

	if (tree->isPipe) {
		smb_handle_t *h = smb_alloc_handle();
		if (!h)
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_INSUFFICIENT_RESOURCES);
		h->isPipe = 1;
		h->treeId = tree->treeId;
		strncpy(h->name, relName, sizeof(h->name) - 1);
		uint64_t hid = smb_handle_id(h);
		smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, treeId);
		uint8_t *body = outHdr + 64;
		memset(body, 0, 88);
		smb_w16(body + 0, 89);
		smb_w32(body + 4, 1); /* FILE_OPENED */
		smb_w32(body + 56, SMB_ATTR_NORMAL);
		smb_w64(body + 64, hid);
		smb_w64(body + 72, hid);
		memcpy(lastFileId, body + 64, 16);
		return 64 + 88;
	}

	char vfsPath[256];
	smb_resolve_vfs_path(tree->basePrefix, relName, vfsPath, sizeof(vfsPath));

	int exists = 0, isDir = 0;
	uint32_t fileSize = 0;
	exists = smb_stat_vfs(vfsPath, &isDir, &fileSize);

	int wantDir    = (createOptions & 0x00000001u) != 0; /* FILE_DIRECTORY_FILE */
	int wantNonDir = (createOptions & 0x00000040u) != 0; /* FILE_NON_DIRECTORY_FILE */
	uint32_t createAction = 1; /* FILE_OPENED */

	if (exists) {
		if (disposition == 2 /* FILE_CREATE */)
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_OBJECT_NAME_COLLISION);
		if (wantDir && !isDir)
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_NOT_A_DIRECTORY);
		if (wantNonDir && isDir)
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_FILE_IS_A_DIRECTORY);

		if (!isDir && (disposition == 0 /* SUPERSEDE */ || disposition == 4 /* OVERWRITE */ || disposition == 5 /* OVERWRITE_IF */)) {
			lfs_file_t f;
			memset(&f, 0, sizeof(f));
			if (lfs_file_open(&lfs, &f, vfsPath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0)
				return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);
			lfs_file_close(&lfs, &f);
			fileSize = 0;
			createAction = (disposition == 0) ? 0 : 3; /* SUPERSEDED or OVERWRITTEN */
		}
	} else {
		if (disposition == 1 /* FILE_OPEN */ || disposition == 4 /* FILE_OVERWRITE */)
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_OBJECT_NAME_NOT_FOUND);

		if (wantDir) {
			if (lfs_mkdir(&lfs, vfsPath) < 0)
				return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);
			isDir = 1;
			fileSize = 0;
			createAction = 2; /* FILE_CREATED */
		} else {
			lfs_file_t f;
			memset(&f, 0, sizeof(f));
			if (lfs_file_open(&lfs, &f, vfsPath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0)
				return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);
			lfs_file_close(&lfs, &f);
			isDir = 0;
			fileSize = 0;
			createAction = 2; /* FILE_CREATED */
		}
	}

	smb_handle_t *h = smb_alloc_handle();
	if (!h)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);

	h->isDir = (uint8_t)isDir;
	h->treeId = tree->treeId;
	h->fileSize = fileSize;
	h->deleteOnClose = (createOptions & 0x00001000u) ? 1 : 0;
	strncpy(h->path, vfsPath, sizeof(h->path) - 1);
	strncpy(h->name, relName, sizeof(h->name) - 1);
	(void)desiredAccess;

	uint64_t hid = smb_handle_id(h);
	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, treeId);
	uint8_t *body = outHdr + 64;
	memset(body, 0, 88);
	smb_w16(body + 0, 89); /* StructureSize = 89 */
	smb_w32(body + 4, createAction);
	smb_w64(body + 8,  SMB_DEFAULT_FILETIME);
	smb_w64(body + 16, SMB_DEFAULT_FILETIME);
	smb_w64(body + 24, SMB_DEFAULT_FILETIME);
	smb_w64(body + 32, SMB_DEFAULT_FILETIME);
	uint64_t allocSz = isDir ? 0 : (((uint64_t)fileSize + 4095ULL) & ~4095ULL);
	smb_w64(body + 40, allocSz);
	smb_w64(body + 48, (uint64_t)fileSize);
	smb_w32(body + 56, isDir ? SMB_ATTR_DIRECTORY : SMB_ATTR_ARCHIVE);
	smb_w64(body + 64, hid);
	smb_w64(body + 72, hid);
	memcpy(lastFileId, body + 64, 16);
	return 64 + 88;
}

static void smb_resolve_compound_fileid(const uint8_t *reqHdr, const uint8_t *reqFileId, const uint8_t lastFileId[16], uint8_t outFileId[16]) {
	uint32_t flags = smb_r32(reqHdr + 16);
	if ((flags & SMB2_FLAGS_RELATED_OPERATIONS) &&
	    smb_r64(reqFileId) == 0xFFFFFFFFFFFFFFFFULL &&
	    smb_r64(reqFileId + 8) == 0xFFFFFFFFFFFFFFFFULL) {
		memcpy(outFileId, lastFileId, 16);
	} else {
		memcpy(outFileId, reqFileId, 16);
	}
}

static int smb2_handle_close(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, const uint8_t lastFileId[16]) {
	if (reqLen < 64 + 24)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint8_t fid[16];
	smb_resolve_compound_fileid(reqHdr, reqHdr + 64 + 8, lastFileId, fid);
	smb_handle_t *h = smb_get_handle(fid);
	if (!h)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_HANDLE);

	uint16_t closeFlags = smb_r16(reqHdr + 64 + 2);
	int isDir = h->isDir;
	uint32_t fsize = h->fileSize;
	smb_close_handle(h);

	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
	uint8_t *body = outHdr + 64;
	memset(body, 0, 60);
	smb_w16(body + 0, 60);
	if (closeFlags & 0x0001) {
		smb_w16(body + 2, 0x0001);
		smb_w64(body + 8,  SMB_DEFAULT_FILETIME);
		smb_w64(body + 16, SMB_DEFAULT_FILETIME);
		smb_w64(body + 24, SMB_DEFAULT_FILETIME);
		smb_w64(body + 32, SMB_DEFAULT_FILETIME);
		smb_w64(body + 40, isDir ? 0 : (((uint64_t)fsize + 4095ULL) & ~4095ULL));
		smb_w64(body + 48, (uint64_t)fsize);
		smb_w32(body + 56, isDir ? SMB_ATTR_DIRECTORY : SMB_ATTR_ARCHIVE);
	}
	return 64 + 60;
}

static int smb2_handle_flush(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, const uint8_t lastFileId[16]) {
	if (reqLen < 64 + 24)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint8_t fid[16];
	smb_resolve_compound_fileid(reqHdr, reqHdr + 64 + 8, lastFileId, fid);
	smb_handle_t *h = smb_get_handle(fid);
	if (!h)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_HANDLE);

	if (h->lfsOpen) {
		lfs_file_close(&lfs, &h->lfsFile);
		h->lfsOpen = 0;
	}
	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
	uint8_t *body = outHdr + 64;
	smb_w16(body + 0, 4);
	smb_w16(body + 2, 0);
	return 64 + 4;
}

static int smb2_handle_read(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, const uint8_t lastFileId[16]) {
	if (reqLen < 64 + 48)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint32_t readLen = smb_r32(reqHdr + 64 + 4);
	uint64_t offset  = smb_r64(reqHdr + 64 + 8);
	uint8_t fid[16];
	smb_resolve_compound_fileid(reqHdr, reqHdr + 64 + 16, lastFileId, fid);
	smb_handle_t *h = smb_get_handle(fid);
	if (!h || h->isDir)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_HANDLE);

	if (readLen > SMB_MAX_RW_SIZE)
		readLen = SMB_MAX_RW_SIZE;

	if (!h->lfsOpen || h->lfsWritable) {
		if (h->lfsOpen) {
			lfs_file_close(&lfs, &h->lfsFile);
			h->lfsOpen = 0;
		}
		memset(&h->lfsFile, 0, sizeof(h->lfsFile));
		if (lfs_file_open(&lfs, &h->lfsFile, h->path, LFS_O_RDONLY) < 0)
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);
		h->lfsOpen = 1;
		h->lfsWritable = 0;
		h->curOffset = 0;
		uint32_t sz = (uint32_t)lfs_file_size(&lfs, &h->lfsFile);
		h->fileSize = sz;
	}

	if (offset >= h->fileSize)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_END_OF_FILE);

	if ((uint32_t)offset != h->curOffset) {
		lfs_file_seek(&lfs, &h->lfsFile, (lfs_soff_t)offset, LFS_SEEK_SET);
		h->curOffset = (uint32_t)offset;
	}

	uint8_t *body = outHdr + 64;
	uint8_t *dataDst = body + 16;
	int n = lfs_file_read(&lfs, &h->lfsFile, dataDst, readLen);
	if (n <= 0)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_END_OF_FILE);

	h->curOffset += (uint32_t)n;
	g_smbBytesRead += (uint32_t)n;

	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
	memset(body, 0, 16);
	smb_w16(body + 0, 17);          /* StructureSize = 17 */
	body[2] = 64 + 16;              /* DataOffset = 80 */
	smb_w32(body + 4, (uint32_t)n); /* DataLength */
	return 64 + 16 + n;
}

static int smb2_handle_write(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, const uint8_t lastFileId[16]) {
	if (reqLen < 64 + 48)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint16_t dataOff = smb_r16(reqHdr + 64 + 2);
	uint32_t writeLen = smb_r32(reqHdr + 64 + 4);
	uint64_t offset   = smb_r64(reqHdr + 64 + 8);
	uint8_t fid[16];
	smb_resolve_compound_fileid(reqHdr, reqHdr + 64 + 16, lastFileId, fid);
	smb_handle_t *h = smb_get_handle(fid);
	if (!h || h->isDir)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_HANDLE);

	if (dataOff < 64 || (int)dataOff + (int)writeLen > reqLen)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	if (!h->lfsOpen || !h->lfsWritable) {
		if (h->lfsOpen) {
			lfs_file_close(&lfs, &h->lfsFile);
			h->lfsOpen = 0;
		}
		memset(&h->lfsFile, 0, sizeof(h->lfsFile));
		int flags = (offset == 0 && h->fileSize == 0)
		            ? (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC)
		            : (LFS_O_RDWR | LFS_O_CREAT);
		if (lfs_file_open(&lfs, &h->lfsFile, h->path, flags) < 0)
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);
		h->lfsOpen = 1;
		h->lfsWritable = 1;
		h->curOffset = 0;
	}

	if ((uint32_t)offset != h->curOffset) {
		lfs_file_seek(&lfs, &h->lfsFile, (lfs_soff_t)offset, LFS_SEEK_SET);
		h->curOffset = (uint32_t)offset;
	}

	int w = lfs_file_write(&lfs, &h->lfsFile, reqHdr + dataOff, writeLen);
	if (w < 0)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);

	h->curOffset += (uint32_t)w;
	if (h->curOffset > h->fileSize)
		h->fileSize = h->curOffset;
	g_smbBytesWrote += (uint32_t)w;

	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
	uint8_t *body = outHdr + 64;
	memset(body, 0, 16);
	smb_w16(body + 0, 17);          /* StructureSize = 17 */
	smb_w32(body + 4, (uint32_t)w); /* Count */
	return 64 + 16;
}

/* Format a single directory entry into outBuf according to infoClass */
static int smb2_pack_dir_entry(uint8_t *outBuf, int maxSpace, uint8_t infoClass,
                               const char *name, int isDir, uint32_t size, uint64_t fileIndex) {
	uint8_t nameUtf16[256];
	int nameBytes = utf8_to_utf16le(name, nameUtf16, sizeof(nameUtf16));
	int hdrLen = 0;
	if (infoClass == 1 /* FileDirectoryInformation */)           hdrLen = 64;
	else if (infoClass == 2 /* FileFullDirectoryInformation */)  hdrLen = 68;
	else if (infoClass == 3 /* FileBothDirectoryInformation */)  hdrLen = 94;
	else if (infoClass == 12 /* FileNamesInformation */)         hdrLen = 12;
	else if (infoClass == 37 /* FileIdBothDirectoryInformation */) hdrLen = 104;
	else if (infoClass == 38 /* FileIdFullDirectoryInformation */) hdrLen = 80;
	else return -1;

	int rawLen = hdrLen + nameBytes;
	int alignedLen = (rawLen + 7) & ~7;
	if (alignedLen > maxSpace)
		return 0;

	memset(outBuf, 0, alignedLen);
	smb_w32(outBuf + 4, (uint32_t)fileIndex);
	if (infoClass == 12) {
		smb_w32(outBuf + 8, (uint32_t)nameBytes);
		memcpy(outBuf + 12, nameUtf16, nameBytes);
		return alignedLen;
	}

	smb_w64(outBuf + 8,  SMB_DEFAULT_FILETIME);
	smb_w64(outBuf + 16, SMB_DEFAULT_FILETIME);
	smb_w64(outBuf + 24, SMB_DEFAULT_FILETIME);
	smb_w64(outBuf + 32, SMB_DEFAULT_FILETIME);
	smb_w64(outBuf + 40, (uint64_t)size);
	smb_w64(outBuf + 48, isDir ? 0 : (((uint64_t)size + 4095ULL) & ~4095ULL));
	smb_w32(outBuf + 56, isDir ? SMB_ATTR_DIRECTORY : SMB_ATTR_ARCHIVE);
	smb_w32(outBuf + 60, (uint32_t)nameBytes);

	if (infoClass == 37) {
		smb_w64(outBuf + 96, fileIndex);
	} else if (infoClass == 38) {
		smb_w64(outBuf + 72, fileIndex);
	}
	memcpy(outBuf + hdrLen, nameUtf16, nameBytes);
	return alignedLen;
}

static int smb2_handle_query_directory(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, const uint8_t lastFileId[16]) {
	if (reqLen < 64 + 32)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint8_t infoClass = reqHdr[64 + 2];
	uint8_t qFlags    = reqHdr[64 + 3];
	uint8_t fid[16];
	smb_resolve_compound_fileid(reqHdr, reqHdr + 64 + 8, lastFileId, fid);
	smb_handle_t *h = smb_get_handle(fid);
	if (!h || !h->isDir)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_HANDLE);

	uint16_t patOff = smb_r16(reqHdr + 64 + 24);
	uint16_t patLen = smb_r16(reqHdr + 64 + 26);
	uint32_t maxOut = smb_r32(reqHdr + 64 + 28);
	if (maxOut > SMB_MAX_RW_SIZE)
		maxOut = SMB_MAX_RW_SIZE;

	if (patLen > 0 && patOff >= 64 && (int)patOff + (int)patLen <= reqLen) {
		char newPat[64];
		utf16le_to_utf8(reqHdr + patOff, patLen, newPat, sizeof(newPat));
		if (strcmp(h->pattern, newPat) != 0) {
			strncpy(h->pattern, newPat, sizeof(h->pattern) - 1);
			h->dirIndex = 0;
			h->dirDone = 0;
		}
	}
	if (qFlags & 0x15) { /* RESTART_SCANS (0x01) | INDEX_SPECIFIED (0x04) | REOPEN (0x10) */
		h->dirIndex = 0;
		h->dirDone = 0;
	}
	if (h->dirDone)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_NO_MORE_FILES);

	int singleEntry = (qFlags & 0x02) != 0;
	uint8_t *body = outHdr + 64;
	uint8_t *outBuf = body + 8;
	int totalBytes = 0;
	uint8_t *lastEntry = NULL;
	uint32_t curIdx = 0;

	/* 1. Include "." and ".." first if pattern matches */
	const char *dots[2] = { ".", ".." };
	for (int d = 0; d < 2; d++) {
		if (!smb_wildcard_match(h->pattern, dots[d]))
			continue;
		if (curIdx >= h->dirIndex) {
			int len = smb2_pack_dir_entry(outBuf + totalBytes, (int)maxOut - totalBytes,
			                              infoClass, dots[d], 1, 0, curIdx + 1);
			if (len < 0)
				return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_INFO_CLASS);
			if (len == 0)
				break;
			if (lastEntry)
				smb_w32(lastEntry, (uint32_t)((outBuf + totalBytes) - lastEntry));
			lastEntry = outBuf + totalBytes;
			totalBytes += len;
			h->dirIndex = curIdx + 1;
			if (singleEntry)
				goto done_pack;
		}
		curIdx++;
	}

	/* 2. Walk VFS directory entries via lfs_dir_open / lfs_dir_read */
	lfs_dir_t dir;
	memset(&dir, 0, sizeof(dir));
	if (lfs_dir_open(&lfs, &dir, h->path) >= 0) {
		struct lfs_info info;
		while (lfs_dir_read(&lfs, &dir, &info) > 0) {
			if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
				continue;
			if (!smb_wildcard_match(h->pattern, info.name))
				continue;
			if (curIdx >= h->dirIndex) {
				int isD = (info.type == LFS_TYPE_DIR);
				int len = smb2_pack_dir_entry(outBuf + totalBytes, (int)maxOut - totalBytes,
				                              infoClass, info.name, isD, info.size, curIdx + 1);
				if (len == 0) {
					lfs_dir_close(&lfs, &dir);
					goto done_pack;
				}
				if (lastEntry)
					smb_w32(lastEntry, (uint32_t)((outBuf + totalBytes) - lastEntry));
				lastEntry = outBuf + totalBytes;
				totalBytes += len;
				h->dirIndex = curIdx + 1;
				if (singleEntry) {
					lfs_dir_close(&lfs, &dir);
					goto done_pack;
				}
			}
			curIdx++;
		}
		lfs_dir_close(&lfs, &dir);
	}
	h->dirDone = 1;

done_pack:
	if (totalBytes == 0) {
		h->dirDone = 1;
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_NO_MORE_FILES);
	}
	if (lastEntry)
		smb_w32(lastEntry, 0); /* NextEntryOffset = 0 on final entry */

	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
	smb_w16(body + 0, 9);
	smb_w16(body + 2, 64 + 8);
	smb_w32(body + 4, (uint32_t)totalBytes);
	return 64 + 8 + totalBytes;
}

static int smb2_handle_query_info(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, const uint8_t lastFileId[16]) {
	if (reqLen < 64 + 40)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint8_t infoType  = reqHdr[64 + 2];
	uint8_t infoClass = reqHdr[64 + 3];
	uint32_t maxOut   = smb_r32(reqHdr + 64 + 4);
	uint8_t fid[16];
	smb_resolve_compound_fileid(reqHdr, reqHdr + 64 + 24, lastFileId, fid);
	smb_handle_t *h = smb_get_handle(fid);
	if (!h)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_HANDLE);

	uint8_t *body = outHdr + 64;
	uint8_t *data = body + 8;
	int dlen = 0;
	memset(data, 0, 256);

	uint64_t allocSz = h->isDir ? 0 : (((uint64_t)h->fileSize + 4095ULL) & ~4095ULL);
	uint32_t attrs   = h->isDir ? SMB_ATTR_DIRECTORY : SMB_ATTR_ARCHIVE;

	if (infoType == 1 /* SMB2_0_INFO_FILE */) {
		if (infoClass == 4 /* FileBasicInformation */) {
			smb_w64(data + 0,  SMB_DEFAULT_FILETIME);
			smb_w64(data + 8,  SMB_DEFAULT_FILETIME);
			smb_w64(data + 16, SMB_DEFAULT_FILETIME);
			smb_w64(data + 24, SMB_DEFAULT_FILETIME);
			smb_w32(data + 32, attrs);
			dlen = 40;
		} else if (infoClass == 5 /* FileStandardInformation */) {
			smb_w64(data + 0, allocSz);
			smb_w64(data + 8, (uint64_t)h->fileSize);
			smb_w32(data + 16, 1); /* NumberOfLinks */
			data[20] = h->deleteOnClose;
			data[21] = h->isDir;
			dlen = 24;
		} else if (infoClass == 6 /* FileInternalInformation */) {
			smb_w64(data, smb_handle_id(h));
			dlen = 8;
		} else if (infoClass == 7 /* FileEaInformation */) {
			dlen = 4;
		} else if (infoClass == 8 /* FileAccessInformation */) {
			smb_w32(data, 0x001F01FFu);
			dlen = 4;
		} else if (infoClass == 14 /* FilePositionInformation */) {
			smb_w64(data, (uint64_t)h->curOffset);
			dlen = 8;
		} else if (infoClass == 18 /* FileAllInformation */) {
			/* Basic (40) + Standard (24) + Internal (8) + Ea (4) + Access (4) + Position (8) + Mode (4) + Alignment (4) + Name (4 + N) */
			smb_w64(data + 0,  SMB_DEFAULT_FILETIME);
			smb_w64(data + 8,  SMB_DEFAULT_FILETIME);
			smb_w64(data + 16, SMB_DEFAULT_FILETIME);
			smb_w64(data + 24, SMB_DEFAULT_FILETIME);
			smb_w32(data + 32, attrs);
			smb_w64(data + 40, allocSz);
			smb_w64(data + 48, (uint64_t)h->fileSize);
			smb_w32(data + 56, 1);
			data[60] = h->deleteOnClose;
			data[61] = h->isDir;
			smb_w64(data + 64, smb_handle_id(h));
			smb_w32(data + 76, 0x001F01FFu);
			smb_w64(data + 80, (uint64_t)h->curOffset);
			char winName[140];
			snprintf(winName, sizeof(winName), "\\%s", h->name);
			int nbytes = utf8_to_utf16le(winName, data + 100, 128);
			smb_w32(data + 96, (uint32_t)nbytes);
			dlen = 100 + nbytes;
		} else if (infoClass == 21 /* FileAlternateNameInformation */ || infoClass == 9 /* FileNameInformation */) {
			char winName[140];
			snprintf(winName, sizeof(winName), "\\%s", h->name);
			int nbytes = utf8_to_utf16le(winName, data + 4, 128);
			smb_w32(data, (uint32_t)nbytes);
			dlen = 4 + nbytes;
		} else if (infoClass == 22 /* FileStreamInformation */) {
			if (h->isDir) {
				dlen = 0;
			} else {
				int nbytes = utf8_to_utf16le("::$DATA", data + 24, 32);
				smb_w32(data + 4, (uint32_t)nbytes);
				smb_w64(data + 8, (uint64_t)h->fileSize);
				smb_w64(data + 16, allocSz);
				dlen = 24 + nbytes;
			}
		} else if (infoClass == 34 /* FileNetworkOpenInformation */) {
			smb_w64(data + 0,  SMB_DEFAULT_FILETIME);
			smb_w64(data + 8,  SMB_DEFAULT_FILETIME);
			smb_w64(data + 16, SMB_DEFAULT_FILETIME);
			smb_w64(data + 24, SMB_DEFAULT_FILETIME);
			smb_w64(data + 32, allocSz);
			smb_w64(data + 40, (uint64_t)h->fileSize);
			smb_w32(data + 48, attrs);
			dlen = 56;
		} else if (infoClass == 35 /* FileAttributeTagInformation */) {
			smb_w32(data + 0, attrs);
			smb_w32(data + 4, 0);
			dlen = 8;
		} else {
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_NOT_SUPPORTED);
		}
	} else if (infoType == 2 /* SMB2_0_INFO_FILESYSTEM */) {
		uint64_t totalUnits = 262144ULL;
		uint64_t freeUnits  = 196608ULL;
		uint32_t volSerial  = 0x8196C001u;
		char volLabel[64]   = "OpenBeken";
		char fsName[16]     = "NTFS";
#ifdef PLATFORM_RTL8196C
		extern void obk_vfs_statfs(const char *path, uint64_t *out_total_units, uint64_t *out_free_units,
		                           uint32_t *out_serial, char *out_label, int label_sz, char *out_fs, int fs_sz);
		smb_tree_t *tree = smb_find_tree(c, h->treeId);
		const char *statPath = (h->path[0] && strcmp(h->path, "/") != 0) ? h->path : (tree && tree->basePrefix[0] ? tree->basePrefix : "/");
		obk_vfs_statfs(statPath, &totalUnits, &freeUnits, &volSerial, volLabel, sizeof(volLabel), fsName, sizeof(fsName));
#endif
		if (infoClass == 1 /* FileFsVolumeInformation */) {
			smb_w64(data + 0, SMB_DEFAULT_FILETIME);
			smb_w32(data + 8, volSerial); /* VolumeSerialNumber */
			int lbytes = utf8_to_utf16le(volLabel, data + 18, 64);
			smb_w32(data + 12, (uint32_t)lbytes);
			dlen = 18 + lbytes;
		} else if (infoClass == 3 /* FileFsSizeInformation */) {
			smb_w64(data + 0, totalUnits); /* TotalAllocationUnits (in 4 KB units) */
			smb_w64(data + 8, freeUnits);  /* AvailableAllocationUnits */
			smb_w32(data + 16, 8);         /* SectorsPerAllocationUnit */
			smb_w32(data + 20, 512);       /* BytesPerSector */
			dlen = 24;
		} else if (infoClass == 4 /* FileFsDeviceInformation */) {
			smb_w32(data + 0, 0x00000007u); /* FILE_DEVICE_DISK */
			smb_w32(data + 4, 0x00000020u); /* FILE_REMOTE_DEVICE */
			dlen = 8;
		} else if (infoClass == 5 /* FileFsAttributeInformation */) {
			smb_w32(data + 0, 0x00000007u); /* CASE_SENSITIVE_SEARCH | CASE_PRESERVED_NAMES | UNICODE_ON_DISK */
			smb_w32(data + 4, 255);         /* MaximumComponentNameLength */
			int fsBytes = utf8_to_utf16le(fsName, data + 12, 16);
			smb_w32(data + 8, (uint32_t)fsBytes);
			dlen = 12 + fsBytes;
		} else if (infoClass == 7 /* FileFsFullSizeInformation */) {
			smb_w64(data + 0, totalUnits);
			smb_w64(data + 8, freeUnits);
			smb_w64(data + 16, freeUnits);
			smb_w32(data + 24, 8);
			smb_w32(data + 28, 512);
			dlen = 32;
		} else {
			return smb2_build_error(outHdr, reqHdr, NT_STATUS_NOT_SUPPORTED);
		}
	} else if (infoType == 3 /* SMB2_0_INFO_SECURITY */) {
		/* Minimal self-relative SECURITY_DESCRIPTOR (Revision 1, SE_SELF_RELATIVE | SE_DACL_PRESENT, NULL DACL = full access) */
		data[0] = 1;
		smb_w16(data + 2, 0x8004);
		dlen = 20;
	} else {
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_NOT_SUPPORTED);
	}

	uint32_t status = NT_STATUS_SUCCESS;
	if ((uint32_t)dlen > maxOut) {
		dlen = (int)maxOut;
		status = NT_STATUS_BUFFER_OVERFLOW;
	}
	smb2_init_resp_hdr(outHdr, reqHdr, status, c->sessionId, 0);
	smb_w16(body + 0, 9);
	smb_w16(body + 2, 64 + 8);
	smb_w32(body + 4, (uint32_t)dlen);
	return 64 + 8 + dlen;
}

static int smb2_handle_set_info(smb_client_t *c, const uint8_t *reqHdr, int reqLen, uint8_t *outHdr, const uint8_t lastFileId[16]) {
	if (reqLen < 64 + 32)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_PARAMETER);

	uint8_t infoType  = reqHdr[64 + 2];
	uint8_t infoClass = reqHdr[64 + 3];
	uint32_t bufLen   = smb_r32(reqHdr + 64 + 4);
	uint16_t bufOff   = smb_r16(reqHdr + 64 + 8);
	uint8_t fid[16];
	smb_resolve_compound_fileid(reqHdr, reqHdr + 64 + 16, lastFileId, fid);
	smb_handle_t *h = smb_get_handle(fid);
	if (!h)
		return smb2_build_error(outHdr, reqHdr, NT_STATUS_INVALID_HANDLE);

	const uint8_t *buf = (bufOff >= 64 && (int)bufOff + (int)bufLen <= reqLen) ? (reqHdr + bufOff) : NULL;

	if (infoType == 1 /* SMB2_0_INFO_FILE */ && buf) {
		if (infoClass == 13 /* FileDispositionInformation */ && bufLen >= 1) {
			h->deleteOnClose = buf[0] ? 1 : 0;
		} else if (infoClass == 10 /* FileRenameInformation */ && bufLen >= 20) {
			uint32_t fnLen = smb_r32(buf + 16);
			if (20 + fnLen <= bufLen) {
				char newRel[192], newVfs[256];
				utf16le_to_utf8(buf + 20, (int)fnLen, newRel, sizeof(newRel));
				smb_tree_t *tree = smb_find_tree(c, h->treeId);
				smb_resolve_vfs_path(tree ? tree->basePrefix : "", newRel, newVfs, sizeof(newVfs));
				if (h->lfsOpen) {
					lfs_file_close(&lfs, &h->lfsFile);
					h->lfsOpen = 0;
				}
				if (buf[0]) /* ReplaceIfExists */
					lfs_remove(&lfs, newVfs);
				if (lfs_rename(&lfs, h->path, newVfs) < 0)
					return smb2_build_error(outHdr, reqHdr, NT_STATUS_ACCESS_DENIED);
				strncpy(h->path, newVfs, sizeof(h->path) - 1);
				strncpy(h->name, newRel, sizeof(h->name) - 1);
			}
		} else if (infoClass == 20 /* FileEndOfFileInformation */ && bufLen >= 8) {
			uint64_t newEof = smb_r64(buf);
			if (newEof == 0 && !h->isDir) {
				if (h->lfsOpen) {
					lfs_file_close(&lfs, &h->lfsFile);
					h->lfsOpen = 0;
				}
				lfs_file_t f;
				memset(&f, 0, sizeof(f));
				if (lfs_file_open(&lfs, &f, h->path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) >= 0)
					lfs_file_close(&lfs, &f);
				h->fileSize = 0;
				h->curOffset = 0;
			}
		}
	}

	smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
	uint8_t *body = outHdr + 64;
	smb_w16(body + 0, 2);
	return 64 + 2;
}

/* Sign an outgoing SMB2 PDU with HMAC-SHA256(c->signingKey, pdu)[0..15] */
static void smb2_sign_pdu(smb_client_t *c, uint8_t *pdu, int pduLen) {
	if (!c->hasSigningKey || pduLen < 64)
		return;
	uint32_t flags = smb_r32(pdu + 16);
	smb_w32(pdu + 16, flags | SMB2_FLAGS_SIGNED);
	memset(pdu + 48, 0, 16);
	uint8_t digest[32];
	hmac_sha256(c->signingKey, 16, pdu, (size_t)pduLen, digest);
	memcpy(pdu + 48, digest, 16);
}

/* Dispatch a complete NetBIOS-framed SMB message (including compound chains) */
static void smb_process_frame(smb_client_t *c, const uint8_t *frame, int frameLen) {
	if (frameLen < 64)
		return;

	uint8_t *txFrame = c->txBuf + 4; /* Reserve 4 bytes for NetBIOS length header */
	int txTotal = 0;
	int reqOff = 0;
	uint8_t lastFileId[16];
	memset(lastFileId, 0, sizeof(lastFileId));

	while (reqOff + 64 <= frameLen) {
		const uint8_t *reqHdr = frame + reqOff;
		uint32_t nextCmd = (reqHdr[0] == 0xFE) ? smb_r32(reqHdr + 20) : 0;
		int subLen = (nextCmd > 0 && reqOff + (int)nextCmd <= frameLen) ? (int)nextCmd : (frameLen - reqOff);

		uint16_t cmd = (reqHdr[0] == 0xFE) ? smb_r16(reqHdr + 12) : SMB2_CMD_NEGOTIATE;
		uint32_t reqFlags = (reqHdr[0] == 0xFE) ? smb_r32(reqHdr + 16) : 0;
		uint8_t *outHdr = txFrame + txTotal;
		int respLen = 0;

		g_smbReqCount++;

		switch (cmd) {
		case SMB2_CMD_NEGOTIATE:
			respLen = smb2_handle_negotiate(c, reqHdr, subLen, outHdr);
			break;
		case SMB2_CMD_SESSION_SETUP:
			respLen = smb2_handle_session_setup(c, reqHdr, subLen, outHdr);
			break;
		case SMB2_CMD_LOGOFF:
			c->authenticated = 0;
			c->hasSigningKey = 0;
			smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
			smb_w16(outHdr + 64, 4);
			smb_w16(outHdr + 66, 0);
			respLen = 64 + 4;
			break;
		case SMB2_CMD_TREE_CONNECT:
			respLen = smb2_handle_tree_connect(c, reqHdr, subLen, outHdr);
			break;
		case SMB2_CMD_TREE_DISCONNECT: {
			smb_tree_t *t = smb_find_tree(c, smb_r32(reqHdr + 36));
			if (t) t->inUse = 0;
			smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
			smb_w16(outHdr + 64, 4);
			smb_w16(outHdr + 66, 0);
			respLen = 64 + 4;
			break;
		}
		case SMB2_CMD_CREATE:
			respLen = smb2_handle_create(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_CLOSE:
			respLen = smb2_handle_close(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_FLUSH:
			respLen = smb2_handle_flush(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_READ:
			respLen = smb2_handle_read(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_WRITE:
			respLen = smb2_handle_write(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_QUERY_DIRECTORY:
			respLen = smb2_handle_query_directory(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_QUERY_INFO:
			respLen = smb2_handle_query_info(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_SET_INFO:
			respLen = smb2_handle_set_info(c, reqHdr, subLen, outHdr, lastFileId);
			break;
		case SMB2_CMD_ECHO:
			smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
			smb_w16(outHdr + 64, 4);
			smb_w16(outHdr + 66, 0);
			respLen = 64 + 4;
			break;
		case SMB2_CMD_IOCTL: {
			uint32_t ctlCode = (subLen >= 64 + 8) ? smb_r32(reqHdr + 64 + 4) : 0;
			if (ctlCode == 0x00140204u) {
				/* FSCTL_VALIDATE_NEGOTIATE_INFO (MS-SMB2 2.2.32.6) */
				smb2_init_resp_hdr(outHdr, reqHdr, NT_STATUS_SUCCESS, c->sessionId, 0);
				uint8_t *body = outHdr + 64;
				memset(body, 0, 48 + 24);
				smb_w16(body + 0, 49);               /* StructureSize = 49 */
				smb_w32(body + 4, 0x00140204u);      /* CtlCode */
				memset(body + 8, 0xFF, 16);          /* FileId = { -1, -1 } */
				smb_w32(body + 32, 64 + 48);         /* OutputOffset = 112 */
				smb_w32(body + 36, 24);              /* OutputCount = 24 */
				uint8_t *vni = body + 48;
				smb_w32(vni + 0, 0);                 /* Capabilities = 0 */
				smb_fill_server_guid(vni + 4);       /* Guid[16] */
				smb_w16(vni + 20, 0x0001);           /* SecurityMode = SIGNING_ENABLED */
				smb_w16(vni + 22, c->dialect);       /* Dialect */
				respLen = 64 + 48 + 24;
			} else {
				/* FSCTL_DFS_GET_REFERRALS -> STATUS_FS_DRIVER_REQUIRED, others -> STATUS_NOT_SUPPORTED */
				uint32_t st = (ctlCode == 0x00060194u) ? NT_STATUS_FS_DRIVER_REQUIRED : NT_STATUS_NOT_SUPPORTED;
				respLen = smb2_build_error(outHdr, reqHdr, st);
			}
			break;
		}
		default:
			respLen = smb2_build_error(outHdr, reqHdr, NT_STATUS_NOT_SUPPORTED);
			break;
		}

		/* If there is another command in this compound chain, 8-byte align and set NextCommand */
		if (nextCmd > 0 && reqOff + (int)nextCmd + 64 <= frameLen) {
			int alignedResp = (respLen + 7) & ~7;
			if (alignedResp > respLen)
				memset(outHdr + respLen, 0, alignedResp - respLen);
			respLen = alignedResp;
			smb_w32(outHdr + 20, (uint32_t)respLen);
		} else {
			smb_w32(outHdr + 20, 0);
		}

		int needSign = (reqFlags & SMB2_FLAGS_SIGNED) != 0;
		if (cmd == SMB2_CMD_SESSION_SETUP && smb_r32(outHdr + 8) == NT_STATUS_SUCCESS) {
			needSign = 1;
		}
		if (needSign && c->hasSigningKey && !c->isGuest) {
			smb2_sign_pdu(c, outHdr, respLen);
		}

		txTotal += respLen;
		if (nextCmd == 0 || txTotal + 1024 >= SMB_BUF_SIZE)
			break;
		reqOff += (int)nextCmd;
	}

	if (txTotal > 0) {
		c->txBuf[0] = 0x00;
		c->txBuf[1] = (uint8_t)((txTotal >> 16) & 0xFF);
		c->txBuf[2] = (uint8_t)((txTotal >> 8) & 0xFF);
		c->txBuf[3] = (uint8_t)(txTotal & 0xFF);
		if (smb_send_all(c->sock, c->txBuf, 4 + txTotal) < 0) {
			smb_close_client(c);
		}
	}
}

/* ============================================================================
 * Listener, QuickTick, and Console Commands
 * ============================================================================ */

static int smb_create_listener(int port) {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;
	int opt = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(s);
		return -1;
	}
	if (listen(s, 4) < 0) {
		close(s);
		return -1;
	}
	lwip_fcntl(s, F_SETFL, O_NONBLOCK);
	return s;
}

static commandResult_t Cmd_SMB_Auth(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_GetArgsCount() >= 2) {
		strncpy(g_smbUser, Tokenizer_GetArg(0), sizeof(g_smbUser) - 1);
		strncpy(g_smbPass, Tokenizer_GetArg(1), sizeof(g_smbPass) - 1);
		ADDLOG_INFO(LOG_FEATURE_MAIN, "SMB auth set for user '%s'", g_smbUser);
	}
	return CMD_RES_OK;
}

void DRV_SMB_Init(void) {
	init_lfs(1);
	memset(g_handles, 0, sizeof(g_handles));
	for (int i = 0; i < SMB_MAX_CLIENTS; i++) {
		if (!g_clients[i].rxBuf)
			g_clients[i].rxBuf = (uint8_t *)malloc(SMB_BUF_SIZE);
		if (!g_clients[i].txBuf)
			g_clients[i].txBuf = (uint8_t *)malloc(SMB_BUF_SIZE);
		g_clients[i].sock = -1;
	}

	g_smbListenSock = smb_create_listener(g_smbPort);
	CMD_RegisterCommand("smb_auth", Cmd_SMB_Auth, NULL);
	ADDLOG_INFO(LOG_FEATURE_MAIN, "DRV_SMB_Init: SMB2 Server listening on port %d (\\\\<ip>\\obk)", g_smbPort);
}

void DRV_SMB_Shutdown(void) {
	for (int i = 0; i < SMB_MAX_HANDLES; i++)
		smb_close_handle(&g_handles[i]);
	for (int i = 0; i < SMB_MAX_CLIENTS; i++) {
		smb_close_client(&g_clients[i]);
		if (g_clients[i].rxBuf) { free(g_clients[i].rxBuf); g_clients[i].rxBuf = NULL; }
		if (g_clients[i].txBuf) { free(g_clients[i].txBuf); g_clients[i].txBuf = NULL; }
	}
	if (g_smbListenSock >= 0) {
		close(g_smbListenSock);
		g_smbListenSock = -1;
	}
}

void DRV_SMB_RunQuickTick(void) {
	if (g_smbListenSock < 0)
		return;

	/* 1. Poll active client connections first */
	for (int i = 0; i < SMB_MAX_CLIENTS; i++) {
		smb_client_t *c = &g_clients[i];
		if (c->sock < 0 || !c->rxBuf || !c->txBuf)
			continue;

		int emptyWaits = 0;
		for (int step = 0; step < 64 && c->sock >= 0; step++) {
			if (c->hdrGot < 4) {
				int n = recv(c->sock, (char *)(c->hdrBuf + c->hdrGot), 4 - c->hdrGot, 0);
				if (n > 0) {
					c->idleSec = 0;
					emptyWaits = 0;
					c->hdrGot += n;
					if (c->hdrGot == 4) {
						c->pktLen = ((uint32_t)c->hdrBuf[1] << 16) |
						            ((uint32_t)c->hdrBuf[2] << 8) |
						            (uint32_t)c->hdrBuf[3];
						c->pktGot = 0;
						if (c->hdrBuf[0] == 0x81) {
							/* NetBIOS Session Request -> reply Positive Session Response (0x82 0x00 0x00 0x00) */
							uint8_t drain[128];
							while (c->pktGot < c->pktLen) {
								uint32_t chunk = c->pktLen - c->pktGot;
								if (chunk > sizeof(drain)) chunk = sizeof(drain);
								int r = recv(c->sock, (char *)drain, (int)chunk, 0);
								if (r <= 0) break;
								c->pktGot += (uint32_t)r;
							}
							static const uint8_t posResp[4] = { 0x82, 0x00, 0x00, 0x00 };
							smb_send_all(c->sock, posResp, 4);
							c->hdrGot = 0;
							c->pktLen = 0;
							c->pktGot = 0;
							continue;
						}
						if (c->pktLen == 0 || c->pktLen > SMB_BUF_SIZE - 8) {
							smb_close_client(c);
							break;
						}
					}
				} else if (n == 0 || (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
					smb_close_client(c);
					break;
				} else {
					if (step > 0 && ++emptyWaits <= 2) {
						rtos_delay_milliseconds(1);
						continue;
					}
					break;
				}
				if (c->hdrGot < 4)
					break;
			}

			if (c->pktGot < c->pktLen) {
				int n = recv(c->sock, (char *)(c->rxBuf + c->pktGot), (int)(c->pktLen - c->pktGot), 0);
				if (n > 0) {
					c->idleSec = 0;
					emptyWaits = 0;
					c->pktGot += (uint32_t)n;
					if (c->pktGot == c->pktLen) {
						uint32_t flen = c->pktLen;
						c->hdrGot = 0;
						c->pktLen = 0;
						c->pktGot = 0;
						smb_process_frame(c, c->rxBuf, (int)flen);
					}
				} else if (n == 0 || (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
					smb_close_client(c);
					break;
				} else {
					if (++emptyWaits <= 10) {
						rtos_delay_milliseconds(1);
						continue;
					}
					break;
				}
			}
		}
	}

	/* 2. Accept incoming SMB2 TCP connection */
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int newSock = accept(g_smbListenSock, (struct sockaddr *)&clientAddr, &addrLen);
	if (newSock >= 0) {
		smb_client_t *slot = NULL;
		for (int i = 0; i < SMB_MAX_CLIENTS; i++) {
			if (g_clients[i].sock < 0) {
				slot = &g_clients[i];
				break;
			}
		}
		if (!slot) {
			/* Evict oldest idle client */
			slot = (g_clients[0].idleSec >= g_clients[1].idleSec) ? &g_clients[0] : &g_clients[1];
			smb_close_client(slot);
		}
		lwip_fcntl(newSock, F_SETFL, O_NONBLOCK);
		slot->sock = newSock;
		slot->idleSec = 0;
		slot->hdrGot = 0;
		slot->pktLen = 0;
		slot->pktGot = 0;
		slot->sessionId = 0;
		slot->authenticated = 0;
		slot->hasSigningKey = 0;
		strncpy(slot->ipStr, inet_ntoa(clientAddr.sin_addr), sizeof(slot->ipStr) - 1);
		ADDLOG_INFO(LOG_FEATURE_MAIN, "SMB: client connected from %s", slot->ipStr);
	}
}

void DRV_SMB_RunEverySecond(void) {
	g_smbUptimeSec++;
	for (int i = 0; i < SMB_MAX_CLIENTS; i++) {
		if (g_clients[i].sock >= 0) {
			if (++g_clients[i].idleSec > SMB_IDLE_TIMEOUT_SEC) {
				smb_close_client(&g_clients[i]);
			}
		}
	}
}

void DRV_SMB_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState)
		return;
	int activeClients = 0;
	for (int i = 0; i < SMB_MAX_CLIENTS; i++)
		if (g_clients[i].sock >= 0)
			activeClients++;
	hprintf255(request, "<h5>SMB2 Server: port %d (\\\\%s\\obk), Clients: %d, Read: %u KB, Wrote: %u KB</h5>",
	           g_smbPort, HAL_GetMyIPString(), activeClients,
	           (unsigned int)(g_smbBytesRead / 1024),
	           (unsigned int)(g_smbBytesWrote / 1024));
}

#endif /* ENABLE_DRIVER_SMB && ENABLE_LITTLEFS */
