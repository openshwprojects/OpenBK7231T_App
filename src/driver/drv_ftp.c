/*
 * drv_ftp.c - Cross-platform RFC 959 / RFC 2428 / RFC 3659 FTP Server Driver for OpenBeken
 *
 * Works transparently across LittleFS and any VFS-mounted volumes (e.g. /usb0, /usb1, /usb2
 * FAT16/FAT32/NTFS on RTL8196C) via our_lfs.h without platform-specific storage branches.
 * Streams file uploads (STOR/APPE) and downloads (RETR) in bounded chunks without buffering
 * full files in RAM.
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
#include "drv_ftp.h"

#if ENABLE_DRIVER_FTP && ENABLE_LITTLEFS

#if !PLATFORM_RTL8196C
// rtos_yield() is provided by the RTL8196C SDK; other platforms fall back to a short sleep
#define rtos_yield() rtos_delay_milliseconds(1)
#endif

#if PLATFORM_RTL8196C || WINDOWS
#define FTP_XFER_CHUNK_SIZE 16384
#else
#define FTP_XFER_CHUNK_SIZE 2048
#endif

#define FTP_DEFAULT_CTRL_PORT 21
#define FTP_DEFAULT_PASV_PORT 2121
#define FTP_IDLE_TIMEOUT_SEC  180
#define FTP_DATA_TIMEOUT_SEC  20

enum {
	FTP_XFER_NONE = 0,
	FTP_XFER_RETR = 1,
	FTP_XFER_STOR = 2
};

typedef struct ftp_session_s {
	int        ctrlSock;
	int        dataSock;
	int        authenticated;
	int        userOk;
	int        pasvWaiting;
	int        portActive;
	uint8_t    portIp[4];
	uint16_t   portPort;
	uint8_t    clientIp[4];
	char       clientIpStr[20];
	char       cwd[256];
	char       rnfrPath[256];
	int        hasRnfr;
	uint32_t   restOffset;
	int        xferMode;
	lfs_file_t lfsFile;
	int        lfsFileOpen;
	uint8_t   *xferBuf;
	int        xferBufLen;
	int        xferBufOff;
	uint32_t   xferBytes;
	int        idleSec;
	int        dataIdleSec;
	char       cmdBuf[320];
	int        cmdLen;
	/* Static scratch buffers to avoid stack usage in quick_timer_thread */
	char       pathAbs[256];
	char       pathVfs[256];
	char       lineBuf[384];
} ftp_session_t;

static int           g_ctrlListenSock = -1;
static int           g_pasvListenSock = -1;
static int           g_ftpCtrlPort    = FTP_DEFAULT_CTRL_PORT;
static int           g_ftpPasvPort    = FTP_DEFAULT_PASV_PORT;
static int           g_ftpUptimeSec   = 0;
static uint32_t      g_ftpBytesSent   = 0;
static uint32_t      g_ftpBytesRecv   = 0;
static uint32_t      g_ftpFilesSent   = 0;
static uint32_t      g_ftpFilesRecv   = 0;
static char          g_ftpUser[64]    = "";
static char          g_ftpPass[64]    = "";
static ftp_session_t g_ftp;

extern const char *HAL_GetMyIPString(void);

static int ftp_send_all(int sock, const void *buf, int len) {
	const uint8_t *p = (const uint8_t *)buf;
	int sent = 0;
	int spins = 0;
	while (sent < len) {
		int r = send(sock, (const char *)(p + sent), len - sent, 0);
		if (r > 0) {
			sent += r;
			spins = 0;
		} else {
			if (++spins > 400)
				return -1;
			rtos_delay_milliseconds(2);
		}
	}
	return sent;
}

static void ftp_reply(const char *fmt, ...) {
	if (g_ftp.ctrlSock < 0)
		return;
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(g_ftp.lineBuf, sizeof(g_ftp.lineBuf) - 3, fmt, ap);
	va_end(ap);
	if (n < 0)
		return;
	if (n > (int)sizeof(g_ftp.lineBuf) - 3)
		n = (int)sizeof(g_ftp.lineBuf) - 3;
	g_ftp.lineBuf[n++] = '\r';
	g_ftp.lineBuf[n++] = '\n';
	g_ftp.lineBuf[n]   = '\0';
	ftp_send_all(g_ftp.ctrlSock, g_ftp.lineBuf, n);
}

static void ftp_close_data(void) {
	if (g_ftp.lfsFileOpen) {
		lfs_file_close(&lfs, &g_ftp.lfsFile);
		g_ftp.lfsFileOpen = 0;
	}
	if (g_ftp.xferBuf) {
		free(g_ftp.xferBuf);
		g_ftp.xferBuf = NULL;
	}
	g_ftp.xferBufLen = 0;
	g_ftp.xferBufOff = 0;
	g_ftp.xferMode   = FTP_XFER_NONE;
	if (g_ftp.dataSock >= 0) {
		close(g_ftp.dataSock);
		g_ftp.dataSock = -1;
	}
	g_ftp.pasvWaiting = 0;
	g_ftp.portActive  = 0;
	g_ftp.restOffset  = 0;
}

static void ftp_close_session(void) {
	ftp_close_data();
	if (g_ftp.ctrlSock >= 0) {
		close(g_ftp.ctrlSock);
		g_ftp.ctrlSock = -1;
	}
	g_ftp.authenticated = 0;
	g_ftp.userOk        = 0;
	g_ftp.hasRnfr       = 0;
	g_ftp.cmdLen        = 0;
	strcpy(g_ftp.cwd, "/");
}

static int ftp_create_listener(int port) {
	struct sockaddr_in addr;
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
		return -1;

	int flag = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port        = htons((uint16_t)port);

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

static void ftp_drain_pasv_listener(void) {
	if (g_pasvListenSock < 0)
		return;
	struct sockaddr_in addr;
	socklen_t alen = sizeof(addr);
	int s;
	while ((s = accept(g_pasvListenSock, (struct sockaddr *)&addr, &alen)) >= 0) {
		close(s);
		alen = sizeof(addr);
	}
}

/*
 * Normalize a relative or absolute FTP path into:
 *   abs_out: canonical path starting with '/' (e.g. "/", "/usb0", "/usb0/file.jpg")
 *   vfs_out: path suitable for lfs_* / obk_vfs_* (e.g. "", "usb0", "usb0/file.jpg")
 */
static void ftp_resolve_path(const char *base_cwd, const char *arg,
                             char *abs_out, size_t abs_sz,
                             char *vfs_out, size_t vfs_sz) {
	char raw[256];
	if (!arg || !arg[0]) {
		strncpy(raw, base_cwd, sizeof(raw) - 1);
		raw[sizeof(raw) - 1] = '\0';
	} else if (arg[0] == '/') {
		strncpy(raw, arg, sizeof(raw) - 1);
		raw[sizeof(raw) - 1] = '\0';
	} else {
		if (strcmp(base_cwd, "/") == 0)
			snprintf(raw, sizeof(raw), "/%s", arg);
		else
			snprintf(raw, sizeof(raw), "%s/%s", base_cwd, arg);
	}

	/* Split and collapse "." and ".." in-place */
	char *segs[32];
	int nseg = 0;
	char *p = raw;
	while (*p) {
		while (*p == '/' || *p == '\\')
			p++;
		if (!*p)
			break;
		char *start = p;
		while (*p && *p != '/' && *p != '\\')
			p++;
		if (*p)
			*p++ = '\0';
		if (strcmp(start, ".") == 0)
			continue;
		if (strcmp(start, "..") == 0) {
			if (nseg > 0)
				nseg--;
			continue;
		}
		if (nseg < 32)
			segs[nseg++] = start;
	}

	if (nseg == 0) {
		strncpy(abs_out, "/", abs_sz);
		if (vfs_out && vfs_sz > 0)
			vfs_out[0] = '\0';
		return;
	}

	size_t pos = 0;
	for (int i = 0; i < nseg; i++) {
		size_t slen = strlen(segs[i]);
		if (pos + 1 + slen + 1 >= abs_sz)
			break;
		abs_out[pos++] = '/';
		memcpy(abs_out + pos, segs[i], slen);
		pos += slen;
	}
	abs_out[pos] = '\0';

	if (vfs_out && vfs_sz > 0) {
		const char *vp = (abs_out[0] == '/') ? (abs_out + 1) : abs_out;
		strncpy(vfs_out, vp, vfs_sz - 1);
		vfs_out[vfs_sz - 1] = '\0';
	}
}

static int ftp_open_data_socket(void) {
	if (g_ftp.dataSock >= 0)
		return g_ftp.dataSock;

	if (g_ftp.pasvWaiting && g_pasvListenSock >= 0) {
		struct sockaddr_in addr;
		socklen_t alen = sizeof(addr);
		for (int wait = 0; wait < 400; wait++) {
			int s = accept(g_pasvListenSock, (struct sockaddr *)&addr, &alen);
			if (s >= 0) {
				lwip_fcntl(s, F_SETFL, O_NONBLOCK);
				g_ftp.dataSock    = s;
				g_ftp.pasvWaiting = 0;
				return s;
			}
			rtos_delay_milliseconds(5);
		}
		g_ftp.pasvWaiting = 0;
		return -1;
	}

	if (g_ftp.portActive) {
		int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s < 0) {
			g_ftp.portActive = 0;
			return -1;
		}
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port   = htons(g_ftp.portPort);
		memcpy(&addr.sin_addr.s_addr, g_ftp.portIp, 4);
		if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			close(s);
			g_ftp.portActive = 0;
			return -1;
		}
		lwip_fcntl(s, F_SETFL, O_NONBLOCK);
		g_ftp.dataSock   = s;
		g_ftp.portActive = 0;
		return s;
	}

	return -1;
}

static void ftp_get_pasv_ip(uint8_t ip_out[4]) {
	/* If client connected via Soft-AP subnet 192.168.4.x, advertise 192.168.4.1 */
	if (g_ftp.clientIp[0] == 192 && g_ftp.clientIp[1] == 168 && g_ftp.clientIp[2] == 4) {
		ip_out[0] = 192;
		ip_out[1] = 168;
		ip_out[2] = 4;
		ip_out[3] = 1;
		return;
	}
	const char *my_ip = HAL_GetMyIPString();
	int a = 192, b = 168, c = 0, d = 134;
	if (my_ip && sscanf(my_ip, "%d.%d.%d.%d", &a, &b, &c, &d) == 4 &&
	    (a != 0 || b != 0 || c != 0 || d != 0)) {
		ip_out[0] = (uint8_t)a;
		ip_out[1] = (uint8_t)b;
		ip_out[2] = (uint8_t)c;
		ip_out[2] = (uint8_t)c;
		ip_out[3] = (uint8_t)d;
		return;
	}
	ip_out[0] = 192;
	ip_out[1] = 168;
	ip_out[2] = 0;
	ip_out[3] = 196;
}

static const char *ftp_skip_list_flags(const char *arg) {
	if (!arg)
		return "";
	while (*arg == ' ')
		arg++;
	while (*arg == '-') {
		while (*arg && *arg != ' ')
			arg++;
		while (*arg == ' ')
			arg++;
	}
	return arg;
}

static void ftp_cmd_list(const char *arg, int names_only, int mlsd_fmt) {
	init_lfs(1);
	if (!lfs_present()) {
		ftp_reply("550 Filesystem not available.");
		return;
	}

	const char *path_arg = ftp_skip_list_flags(arg);
	ftp_resolve_path(g_ftp.cwd, path_arg,
	                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
	                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));

	lfs_dir_t dir;
	memset(&dir, 0, sizeof(dir));
	int rc = lfs_dir_open(&lfs, &dir, g_ftp.pathVfs);
	if (rc < 0) {
		ftp_reply("550 Failed to open directory \"%s\".", g_ftp.pathAbs);
		return;
	}

	ftp_reply("150 Opening ASCII mode data connection for directory listing.");
	int dsock = ftp_open_data_socket();
	if (dsock < 0) {
		lfs_dir_close(&lfs, &dir);
		ftp_reply("425 Can't open data connection.");
		return;
	}

	struct lfs_info info;
	while (lfs_dir_read(&lfs, &dir, &info) > 0) {
		if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
			continue;
		int n = 0;
		if (names_only) {
			n = snprintf(g_ftp.lineBuf, sizeof(g_ftp.lineBuf), "%s\r\n", info.name);
		} else if (mlsd_fmt) {
			n = snprintf(g_ftp.lineBuf, sizeof(g_ftp.lineBuf),
			             "type=%s;size=%u;modify=20261017120000; %s\r\n",
			             (info.type == LFS_TYPE_DIR) ? "dir" : "file",
			             (unsigned int)info.size, info.name);
		} else {
			n = snprintf(g_ftp.lineBuf, sizeof(g_ftp.lineBuf),
			             "%s   1 owner    group    %12u Oct 17 12:00 %s\r\n",
			             (info.type == LFS_TYPE_DIR) ? "drwxr-xr-x" : "-rw-r--r--",
			             (unsigned int)info.size, info.name);
		}
		if (n > 0) {
			if (ftp_send_all(dsock, g_ftp.lineBuf, n) < 0)
				break;
			g_ftpBytesSent += (uint32_t)n;
		}
	}

	lfs_dir_close(&lfs, &dir);
	ftp_close_data();
	ftp_reply("226 Directory send OK.");
}

static void ftp_cmd_retr(const char *arg) {
	if (!arg || !arg[0]) {
		ftp_reply("501 Missing file name.");
		return;
	}
	init_lfs(1);
	if (!lfs_present()) {
		ftp_reply("550 Filesystem not available.");
		return;
	}

	ftp_resolve_path(g_ftp.cwd, arg,
	                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
	                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));

	memset(&g_ftp.lfsFile, 0, sizeof(g_ftp.lfsFile));
	int rc = lfs_file_open(&lfs, &g_ftp.lfsFile, g_ftp.pathVfs, LFS_O_RDONLY);
	if (rc < 0) {
		ftp_reply("550 Failed to open file \"%s\".", g_ftp.pathAbs);
		return;
	}
	g_ftp.lfsFileOpen = 1;

	if (g_ftp.restOffset > 0) {
		lfs_file_seek(&lfs, &g_ftp.lfsFile, (lfs_soff_t)g_ftp.restOffset, LFS_SEEK_SET);
		g_ftp.restOffset = 0;
	}

	lfs_soff_t fsize = lfs_file_size(&lfs, &g_ftp.lfsFile);
	g_ftp.xferBuf = (uint8_t *)malloc(FTP_XFER_CHUNK_SIZE);
	if (!g_ftp.xferBuf) {
		ftp_close_data();
		ftp_reply("451 Out of memory for transfer buffer.");
		return;
	}

	ftp_reply("150 Opening BINARY mode data connection for %s (%d bytes).",
	          g_ftp.pathAbs, (int)(fsize >= 0 ? fsize : 0));

	int dsock = ftp_open_data_socket();
	if (dsock < 0) {
		ftp_close_data();
		ftp_reply("425 Can't open data connection.");
		return;
	}

	g_ftp.xferBufLen  = 0;
	g_ftp.xferBufOff  = 0;
	g_ftp.xferBytes   = 0;
	g_ftp.dataIdleSec = 0;
	g_ftp.xferMode    = FTP_XFER_RETR;
}

static void ftp_cmd_stor(const char *arg, int append_mode) {
	if (!arg || !arg[0]) {
		ftp_reply("501 Missing file name.");
		return;
	}
	init_lfs(1);
	if (!lfs_present()) {
		ftp_reply("550 Filesystem not available.");
		return;
	}

	ftp_resolve_path(g_ftp.cwd, arg,
	                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
	                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));

	int flags = LFS_O_WRONLY | LFS_O_CREAT;
	if (append_mode)
		flags |= LFS_O_APPEND;
	else if (g_ftp.restOffset == 0)
		flags |= LFS_O_TRUNC;

	memset(&g_ftp.lfsFile, 0, sizeof(g_ftp.lfsFile));
	int rc = lfs_file_open(&lfs, &g_ftp.lfsFile, g_ftp.pathVfs, flags);
	if (rc < 0) {
		ftp_reply("553 Could not create file \"%s\" (err %d).", g_ftp.pathAbs, rc);
		return;
	}
	g_ftp.lfsFileOpen = 1;

	if (g_ftp.restOffset > 0 && !append_mode) {
		lfs_file_seek(&lfs, &g_ftp.lfsFile, (lfs_soff_t)g_ftp.restOffset, LFS_SEEK_SET);
		g_ftp.restOffset = 0;
	}

	g_ftp.xferBuf = (uint8_t *)malloc(FTP_XFER_CHUNK_SIZE);
	if (!g_ftp.xferBuf) {
		ftp_close_data();
		ftp_reply("451 Out of memory for transfer buffer.");
		return;
	}

	ftp_reply("150 Ok to send data.");

	int dsock = ftp_open_data_socket();
	if (dsock < 0) {
		ftp_close_data();
		ftp_reply("425 Can't open data connection.");
		return;
	}

	g_ftp.xferBufLen  = 0;
	g_ftp.xferBufOff  = 0;
	g_ftp.xferBytes   = 0;
	g_ftp.dataIdleSec = 0;
	g_ftp.xferMode    = FTP_XFER_STOR;
}

static void ftp_step_transfer(void) {
	if (g_ftp.xferMode == FTP_XFER_RETR) {
		int chunks = 0;
		for (;;) {
			if (g_ftp.xferBufOff >= g_ftp.xferBufLen) {
				lfs_ssize_t n = lfs_file_read(&lfs, &g_ftp.lfsFile, g_ftp.xferBuf, FTP_XFER_CHUNK_SIZE);
				if (n < 0) {
					ftp_close_data();
					ftp_reply("451 Local read error.");
					return;
				}
				if (n == 0) {
					/* EOF reached and all bytes sent */
					g_ftpFilesSent++;
					ftp_close_data();
					ftp_reply("226 Transfer complete.");
					return;
				}
				g_ftp.xferBufLen = (int)n;
				g_ftp.xferBufOff = 0;
			}

			int rem = g_ftp.xferBufLen - g_ftp.xferBufOff;
			int sent = send(g_ftp.dataSock, (const char *)(g_ftp.xferBuf + g_ftp.xferBufOff), rem, 0);
			if (sent > 0) {
				g_ftp.xferBufOff  += sent;
				g_ftp.xferBytes   += (uint32_t)sent;
				g_ftpBytesSent    += (uint32_t)sent;
				g_ftp.dataIdleSec  = 0;
				g_ftp.idleSec      = 0;
				if (g_ftp.xferBufOff >= g_ftp.xferBufLen) {
					chunks++;
					if ((chunks & 3) == 0)
						rtos_yield();
					if (chunks >= 16)
						return; /* yield back to QuickTick after 256 KB slice */
				}
			} else {
				/* Socket send buffer temporarily full; wait briefly or resume next tick */
				return;
			}
		}
	} else if (g_ftp.xferMode == FTP_XFER_STOR) {
		int chunks = 0;
		for (;;) {
			int r = recv(g_ftp.dataSock, (char *)g_ftp.xferBuf, FTP_XFER_CHUNK_SIZE, 0);
			if (r < 0) {
				for (int w = 0; w < 24 && r < 0; w++) {
					rtos_yield();
					r = recv(g_ftp.dataSock, (char *)g_ftp.xferBuf, FTP_XFER_CHUNK_SIZE, 0);
				}
			}
			if (r > 0) {
				lfs_ssize_t w = lfs_file_write(&lfs, &g_ftp.lfsFile, g_ftp.xferBuf, (lfs_size_t)r);
				if (w != (lfs_ssize_t)r) {
					ftp_close_data();
					ftp_reply("452 Error writing file to storage.");
					return;
				}
				g_ftp.xferBytes   += (uint32_t)r;
				g_ftpBytesRecv    += (uint32_t)r;
				g_ftp.dataIdleSec  = 0;
				g_ftp.idleSec      = 0;
				chunks++;
				if ((chunks & 3) == 0)
					rtos_yield();
				if (chunks >= 16)
					return;
			} else if (r == 0) {
				/* Peer closed data socket -> upload complete */
				int close_rc = 0;
				if (g_ftp.lfsFileOpen) {
					close_rc = lfs_file_close(&lfs, &g_ftp.lfsFile);
					g_ftp.lfsFileOpen = 0;
				}
				ftp_close_data();
				if (close_rc < 0) {
					ftp_reply("452 Error flushing file on close.");
				} else {
					g_ftpFilesRecv++;
					ftp_reply("226 Transfer complete.");
				}
				return;
			} else {
				/* EWOULDBLOCK: no data currently queued */
				return;
			}
		}
	}
}

static void ftp_handle_command(char *line) {
	g_ftp.idleSec = 0;

	/* Strip trailing CR/LF */
	int len = (int)strlen(line);
	while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n'))
		line[--len] = '\0';
	if (len == 0)
		return;

	char *cmd = line;
	while (*cmd == ' ')
		cmd++;
	char *arg = cmd;
	while (*arg && *arg != ' ')
		arg++;
	if (*arg) {
		*arg++ = '\0';
		while (*arg == ' ')
			arg++;
	}

	/* Uppercase the command verb */
	for (char *p = cmd; *p; p++) {
		if (*p >= 'a' && *p <= 'z')
			*p = (char)(*p - 'a' + 'A');
	}

	if (strcmp(cmd, "USER") == 0) {
		if (g_ftpUser[0] == '\0' || strcmp(arg, g_ftpUser) == 0 || strcmp(arg, "anonymous") == 0) {
			g_ftp.userOk = 1;
			if (g_ftpPass[0] == '\0') {
				g_ftp.authenticated = 1;
				ftp_reply("230 Login successful.");
			} else {
				ftp_reply("331 Please specify the password.");
			}
		} else {
			g_ftp.userOk = 0;
			ftp_reply("331 Please specify the password.");
		}
		return;
	}

	if (strcmp(cmd, "PASS") == 0) {
		if (g_ftp.userOk && (g_ftpPass[0] == '\0' || strcmp(arg, g_ftpPass) == 0)) {
			g_ftp.authenticated = 1;
			ftp_reply("230 Login successful.");
		} else {
			g_ftp.authenticated = 0;
			ftp_reply("530 Login incorrect.");
		}
		return;
	}

	if (strcmp(cmd, "SYST") == 0) {
		ftp_reply("215 UNIX Type: L8");
		return;
	}

	if (strcmp(cmd, "FEAT") == 0) {
		const char *feat =
			"211-Features:\r\n"
			" PASV\r\n"
			" EPSV\r\n"
			" SIZE\r\n"
			" MDTM\r\n"
			" REST STREAM\r\n"
			" MLSD\r\n"
			" UTF8\r\n"
			"211 End\r\n";
		ftp_send_all(g_ftp.ctrlSock, feat, (int)strlen(feat));
		return;
	}

	if (strcmp(cmd, "OPTS") == 0) {
		ftp_reply("200 Always in UTF8 mode.");
		return;
	}

	if (strcmp(cmd, "NOOP") == 0) {
		ftp_reply("200 NOOP ok.");
		return;
	}

	if (strcmp(cmd, "HELP") == 0) {
		ftp_reply("214 OpenBeken FTP Server ready.");
		return;
	}

	if (strcmp(cmd, "QUIT") == 0) {
		ftp_reply("221 Goodbye.");
		ftp_close_session();
		return;
	}

	if (strcmp(cmd, "ABOR") == 0) {
		if (g_ftp.xferMode != FTP_XFER_NONE) {
			ftp_close_data();
			ftp_reply("426 Transfer aborted.");
		} else {
			ftp_close_data();
		}
		ftp_reply("226 Abort successful.");
		return;
	}

	if (!g_ftp.authenticated) {
		ftp_reply("530 Please login with USER and PASS.");
		return;
	}

	if (strcmp(cmd, "TYPE") == 0 || strcmp(cmd, "MODE") == 0 || strcmp(cmd, "STRU") == 0) {
		ftp_reply("200 Command okay.");
		return;
	}

	if (strcmp(cmd, "PWD") == 0 || strcmp(cmd, "XPWD") == 0) {
		ftp_reply("257 \"%s\" is the current directory.", g_ftp.cwd);
		return;
	}

	if (strcmp(cmd, "CDUP") == 0 || strcmp(cmd, "XCUP") == 0) {
		arg = "..";
		cmd = "CWD";
	}

	if (strcmp(cmd, "CWD") == 0 || strcmp(cmd, "XCWD") == 0) {
		init_lfs(1);
		ftp_resolve_path(g_ftp.cwd, arg,
		                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
		                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));
		if (strcmp(g_ftp.pathAbs, "/") == 0) {
			strcpy(g_ftp.cwd, "/");
			ftp_reply("250 Directory successfully changed to \"/\".");
			return;
		}
		struct lfs_info info;
		if (lfs_stat(&lfs, g_ftp.pathVfs, &info) == 0 && info.type == LFS_TYPE_DIR) {
			strncpy(g_ftp.cwd, g_ftp.pathAbs, sizeof(g_ftp.cwd) - 1);
			g_ftp.cwd[sizeof(g_ftp.cwd) - 1] = '\0';
			ftp_reply("250 Directory successfully changed to \"%s\".", g_ftp.cwd);
		} else {
			ftp_reply("550 Failed to change directory to \"%s\".", g_ftp.pathAbs);
		}
		return;
	}

	if (strcmp(cmd, "PASV") == 0) {
		ftp_close_data();
		ftp_drain_pasv_listener();
		g_ftp.pasvWaiting = 1;
		uint8_t ip[4];
		ftp_get_pasv_ip(ip);
		ftp_reply("227 Entering Passive Mode (%u,%u,%u,%u,%u,%u).",
		          ip[0], ip[1], ip[2], ip[3],
		          (unsigned)(g_ftpPasvPort >> 8), (unsigned)(g_ftpPasvPort & 0xFF));
		return;
	}

	if (strcmp(cmd, "EPSV") == 0) {
		ftp_close_data();
		ftp_drain_pasv_listener();
		g_ftp.pasvWaiting = 1;
		ftp_reply("229 Entering Extended Passive Mode (|||%d|).", g_ftpPasvPort);
		return;
	}

	if (strcmp(cmd, "PORT") == 0) {
		unsigned h1, h2, h3, h4, p1, p2;
		if (sscanf(arg, "%u,%u,%u,%u,%u,%u", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
			ftp_close_data();
			g_ftp.portIp[0]  = (uint8_t)h1;
			g_ftp.portIp[1]  = (uint8_t)h2;
			g_ftp.portIp[2]  = (uint8_t)h3;
			g_ftp.portIp[3]  = (uint8_t)h4;
			g_ftp.portPort   = (uint16_t)((p1 << 8) | (p2 & 0xFF));
			g_ftp.portActive = 1;
			ftp_reply("200 PORT command successful.");
		} else {
			ftp_reply("501 Invalid PORT syntax.");
		}
		return;
	}

	if (strcmp(cmd, "REST") == 0) {
		g_ftp.restOffset = (uint32_t)strtoul(arg, NULL, 10);
		ftp_reply("350 Restarting at %u.", (unsigned)g_ftp.restOffset);
		return;
	}

	if (strcmp(cmd, "LIST") == 0) {
		ftp_cmd_list(arg, 0, 0);
		return;
	}

	if (strcmp(cmd, "NLST") == 0) {
		ftp_cmd_list(arg, 1, 0);
		return;
	}

	if (strcmp(cmd, "MLSD") == 0) {
		ftp_cmd_list(arg, 0, 1);
		return;
	}

	if (strcmp(cmd, "RETR") == 0) {
		ftp_cmd_retr(arg);
		return;
	}

	if (strcmp(cmd, "STOR") == 0) {
		ftp_cmd_stor(arg, 0);
		return;
	}

	if (strcmp(cmd, "APPE") == 0) {
		ftp_cmd_stor(arg, 1);
		return;
	}

	if (strcmp(cmd, "SIZE") == 0) {
		init_lfs(1);
		ftp_resolve_path(g_ftp.cwd, arg,
		                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
		                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));
		struct lfs_info info;
		if (g_ftp.pathVfs[0] != '\0' && lfs_stat(&lfs, g_ftp.pathVfs, &info) == 0 &&
		    info.type == LFS_TYPE_REG) {
			ftp_reply("213 %u", (unsigned)info.size);
		} else {
			ftp_reply("550 Could not get file size.");
		}
		return;
	}

	if (strcmp(cmd, "MDTM") == 0) {
		init_lfs(1);
		ftp_resolve_path(g_ftp.cwd, arg,
		                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
		                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));
		struct lfs_info info;
		if (g_ftp.pathVfs[0] != '\0' && lfs_stat(&lfs, g_ftp.pathVfs, &info) == 0) {
			ftp_reply("213 20261017120000");
		} else {
			ftp_reply("550 File not found.");
		}
		return;
	}

	if (strcmp(cmd, "DELE") == 0 || strcmp(cmd, "RMD") == 0 || strcmp(cmd, "XRMD") == 0) {
		init_lfs(1);
		ftp_resolve_path(g_ftp.cwd, arg,
		                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
		                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));
		if (g_ftp.pathVfs[0] != '\0' && lfs_remove(&lfs, g_ftp.pathVfs) == 0) {
			ftp_reply("250 Delete operation successful.");
		} else {
			ftp_reply("550 Delete operation failed.");
		}
		return;
	}

	if (strcmp(cmd, "MKD") == 0 || strcmp(cmd, "XMKD") == 0) {
		init_lfs(1);
		ftp_resolve_path(g_ftp.cwd, arg,
		                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
		                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));
		if (g_ftp.pathVfs[0] != '\0' && lfs_mkdir(&lfs, g_ftp.pathVfs) == 0) {
			ftp_reply("257 \"%s\" created.", g_ftp.pathAbs);
		} else {
			ftp_reply("550 Create directory operation failed.");
		}
		return;
	}

	if (strcmp(cmd, "RNFR") == 0) {
		init_lfs(1);
		ftp_resolve_path(g_ftp.cwd, arg,
		                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
		                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));
		struct lfs_info info;
		if (g_ftp.pathVfs[0] != '\0' && lfs_stat(&lfs, g_ftp.pathVfs, &info) == 0) {
			strncpy(g_ftp.rnfrPath, g_ftp.pathVfs, sizeof(g_ftp.rnfrPath) - 1);
			g_ftp.rnfrPath[sizeof(g_ftp.rnfrPath) - 1] = '\0';
			g_ftp.hasRnfr = 1;
			ftp_reply("350 Ready for RNTO.");
		} else {
			g_ftp.hasRnfr = 0;
			ftp_reply("550 RNFR target does not exist.");
		}
		return;
	}

	if (strcmp(cmd, "RNTO") == 0) {
		if (!g_ftp.hasRnfr) {
			ftp_reply("503 RNFR required first.");
			return;
		}
		g_ftp.hasRnfr = 0;
		ftp_resolve_path(g_ftp.cwd, arg,
		                 g_ftp.pathAbs, sizeof(g_ftp.pathAbs),
		                 g_ftp.pathVfs, sizeof(g_ftp.pathVfs));
		if (g_ftp.pathVfs[0] != '\0' && lfs_rename(&lfs, g_ftp.rnfrPath, g_ftp.pathVfs) == 0) {
			ftp_reply("250 Rename successful.");
		} else {
			ftp_reply("550 Rename failed.");
		}
		return;
	}

	ftp_reply("502 Command not implemented.");
}

static commandResult_t Cmd_FTP_Port(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() >= 1) {
		int p = Tokenizer_GetArgInteger(0);
		if (p > 0 && p < 65535) {
			g_ftpCtrlPort = p;
			if (g_ctrlListenSock >= 0) {
				close(g_ctrlListenSock);
				g_ctrlListenSock = ftp_create_listener(g_ftpCtrlPort);
			}
		}
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "FTP control port=%d, PASV port=%d", g_ftpCtrlPort, g_ftpPasvPort);
	return CMD_RES_OK;
}

static commandResult_t Cmd_FTP_Auth(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_GetArgsCount() >= 2) {
		strncpy(g_ftpUser, Tokenizer_GetArg(0), sizeof(g_ftpUser) - 1);
		strncpy(g_ftpPass, Tokenizer_GetArg(1), sizeof(g_ftpPass) - 1);
		ADDLOG_INFO(LOG_FEATURE_MAIN, "FTP auth set for user '%s'", g_ftpUser);
	} else {
		g_ftpUser[0] = '\0';
		g_ftpPass[0] = '\0';
		ADDLOG_INFO(LOG_FEATURE_MAIN, "FTP anonymous login enabled");
	}
	return CMD_RES_OK;
}

void DRV_FTP_Init(void) {
	memset(&g_ftp, 0, sizeof(g_ftp));
	g_ftp.ctrlSock = -1;
	g_ftp.dataSock = -1;
	strcpy(g_ftp.cwd, "/");

	init_lfs(1);

	g_ctrlListenSock = ftp_create_listener(g_ftpCtrlPort);
	g_pasvListenSock = ftp_create_listener(g_ftpPasvPort);

	CMD_RegisterCommand("ftp_port", Cmd_FTP_Port, NULL);
	CMD_RegisterCommand("ftp_auth", Cmd_FTP_Auth, NULL);

	ADDLOG_INFO(LOG_FEATURE_MAIN,
	            "DRV_FTP_Init: FTP Server listening on port %d (PASV port %d)",
	            g_ftpCtrlPort, g_ftpPasvPort);
}

void DRV_FTP_RunQuickTick(void) {
	if (g_ctrlListenSock < 0)
		return;

	/* Poll active control connection for FTP commands or disconnect first */
	if (g_ftp.ctrlSock >= 0) {
		int space = (int)sizeof(g_ftp.cmdBuf) - 1 - g_ftp.cmdLen;
		if (space <= 0) {
			g_ftp.cmdLen = 0;
			space = (int)sizeof(g_ftp.cmdBuf) - 1;
		}
		int n = recv(g_ftp.ctrlSock, g_ftp.cmdBuf + g_ftp.cmdLen, space, 0);
		if (n > 0) {
			g_ftp.cmdLen += n;
			g_ftp.cmdBuf[g_ftp.cmdLen] = '\0';
			char *start = g_ftp.cmdBuf;
			char *nl;
			while ((nl = strchr(start, '\n')) != NULL) {
				*nl = '\0';
				ftp_handle_command(start);
				if (g_ftp.ctrlSock < 0)
					break;
				start = nl + 1;
			}
			if (g_ftp.ctrlSock >= 0) {
				int rem = (int)(g_ftp.cmdBuf + g_ftp.cmdLen - start);
				if (rem > 0 && start != g_ftp.cmdBuf)
					memmove(g_ftp.cmdBuf, start, rem);
				g_ftp.cmdLen = rem;
			}
		} else if (n == 0 || (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
			ADDLOG_INFO(LOG_FEATURE_MAIN, "FTP: client %s disconnected", g_ftp.clientIpStr);
			ftp_close_session();
		}
	}

	/* Accept new control connection if none active (or replace idle session) */
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int newSock = accept(g_ctrlListenSock, (struct sockaddr *)&clientAddr, &addrLen);
	if (newSock >= 0) {
		if (g_ftp.ctrlSock >= 0 && g_ftp.xferMode == FTP_XFER_NONE) {
			ftp_close_session();
		}
		if (g_ftp.ctrlSock < 0) {
			lwip_fcntl(newSock, F_SETFL, O_NONBLOCK);
			g_ftp.ctrlSock      = newSock;
			g_ftp.dataSock      = -1;
			g_ftp.authenticated = 0;
			g_ftp.userOk        = 0;
			g_ftp.pasvWaiting   = 0;
			g_ftp.portActive    = 0;
			g_ftp.hasRnfr       = 0;
			g_ftp.restOffset    = 0;
			g_ftp.cmdLen        = 0;
			g_ftp.idleSec       = 0;
			strcpy(g_ftp.cwd, "/");
			memcpy(g_ftp.clientIp, &clientAddr.sin_addr.s_addr, 4);
			strncpy(g_ftp.clientIpStr, inet_ntoa(clientAddr.sin_addr), sizeof(g_ftp.clientIpStr) - 1);
			ADDLOG_INFO(LOG_FEATURE_MAIN, "FTP: client connected from %s", g_ftp.clientIpStr);
			ftp_reply("220 OpenBeken FTP Server (%s) ready.", CFG_GetDeviceName());
		} else {
			const char *busy = "421 Server busy with active transfer.\r\n";
			send(newSock, busy, (int)strlen(busy), 0);
			close(newSock);
		}
	}

	/* Step active streaming transfer (RETR or STOR) */
	if (g_ftp.ctrlSock >= 0 && g_ftp.xferMode != FTP_XFER_NONE) {
		ftp_step_transfer();
	}
}

void DRV_FTP_RunEverySecond(void) {
	g_ftpUptimeSec++;
	if (g_ftp.ctrlSock >= 0) {
		g_ftp.idleSec++;
		if (g_ftp.xferMode != FTP_XFER_NONE) {
			g_ftp.dataIdleSec++;
			if (g_ftp.dataIdleSec > FTP_DATA_TIMEOUT_SEC) {
				ftp_close_data();
				ftp_reply("426 Data transfer timed out.");
			}
		} else if (g_ftp.idleSec > FTP_IDLE_TIMEOUT_SEC) {
			ftp_reply("421 Timeout.");
			ftp_close_session();
		}
	}
}

void DRV_FTP_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState)
		return;
	hprintf255(request,
	           "<h5>FTP Server: ftp://%s:%d/ (TX %u KB / %u files, RX %u KB / %u files, %s)</h5>",
	           HAL_GetMyIPString(), g_ftpCtrlPort,
	           (unsigned)(g_ftpBytesSent >> 10), (unsigned)g_ftpFilesSent,
	           (unsigned)(g_ftpBytesRecv >> 10), (unsigned)g_ftpFilesRecv,
	           (g_ftp.ctrlSock >= 0) ? g_ftp.clientIpStr : "idle");
}

void DRV_FTP_Shutdown(void) {
	ftp_close_session();
	if (g_pasvListenSock >= 0) {
		close(g_pasvListenSock);
		g_pasvListenSock = -1;
	}
	if (g_ctrlListenSock >= 0) {
		close(g_ctrlListenSock);
		g_ctrlListenSock = -1;
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "DRV_FTP_Shutdown: FTP Server stopped");
}

#endif /* ENABLE_DRIVER_FTP && ENABLE_LITTLEFS */
