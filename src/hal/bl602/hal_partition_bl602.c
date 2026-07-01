// BL602 partition table dump command.
//
// Reads the two Bouffalo partition tables straight from physical flash
// (0xE000 / 0xF000), validates them with CRC32, picks the active one and
// prints a human readable layout - the same information that the host side
// bl602_partition_info.py produces from a flash dump:
//  - which firmware slot is active and where the OTA slot lives,
//  - size of every partition,
//  - where the partitioned area ends (free space after it).

#if PLATFORM_BL602 && !PLATFORM_BL_NEW

#include "../../obk_config.h"
#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../cmnds/cmd_public.h"
#include "../hal_ota.h"

// SDK internals for reading the physical flash chip's JEDEC ID. We avoid
// pulling the heavy bl602 std driver headers and just declare what we use.
// XIP_SFlash_GetJedecId_Need_Lock lives in TCM and saves/restores the XIP
// state itself, so we only have to keep interrupts off around it (same as
// bl_flash_read does in the hosal layer).
extern void *bl_flash_get_flashCfg(void);
extern int bl_irq_save(void);
extern void bl_irq_restore(int flags);
extern int XIP_SFlash_GetJedecId_Need_Lock(void *pFlashCfg, uint8_t *data);

// Read 3-byte JEDEC ID (data[0]=manufacturer, data[1]=type, data[2]=capacity).
// Returns 0 on success. data[2] encodes the chip size as (1 << data[2]) bytes.
static int pt_read_jedec_id(uint8_t out[3]) {
	void *cfg = bl_flash_get_flashCfg();
	int flags;
	if (cfg == NULL)
		return -1;
	out[0] = out[1] = out[2] = 0;
	flags = bl_irq_save();
	XIP_SFlash_GetJedecId_Need_Lock(cfg, out);
	bl_irq_restore(flags);
	return 0;
}

#define PT_TABLE0_ADDRESS   0xE000
#define PT_TABLE1_ADDRESS   0xF000
#define PT_MAGIC            0x54504642   // "BFPT"
#define PT_HEADER_SIZE      16
#define PT_ENTRY_SIZE       36
#define PT_ENTRY_MAX        16

typedef struct {
	uint8_t  type;
	uint8_t  device;
	uint8_t  activeIndex;
	char     name[10];      // 9 bytes on flash + room for terminator
	uint32_t address[2];
	uint32_t size[2];
	uint32_t len;
	uint32_t age;
} pt_entry_t;

typedef struct {
	uint32_t offset;
	int      valid;
	const char *error;
	uint32_t magic;
	uint16_t version;
	uint16_t entryCnt;
	uint32_t age;
	uint32_t headerCrc;
	uint32_t entriesCrc;
	pt_entry_t entries[PT_ENTRY_MAX];
} pt_table_t;

static uint16_t rd16(const uint8_t *p) {
	return (uint16_t)(p[0] | (p[1] << 8));
}
static uint32_t rd32(const uint8_t *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Standard zlib/CRC-32 (poly 0xEDB88320), as used by the Python tool.
static uint32_t pt_crc32(const uint8_t *p, int len) {
	uint32_t crc = 0xFFFFFFFF;
	int i, b;
	for (i = 0; i < len; i++) {
		crc ^= p[i];
		for (b = 0; b < 8; b++) {
			crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1)));
		}
	}
	return ~crc;
}

// Format a byte count the way the Python tool does: 0, exact M, exact K, else raw.
static void fmt_size(uint32_t v, char *out, int outlen) {
	if (v == 0)
		snprintf(out, outlen, "0");
	else if ((v % (1024 * 1024)) == 0)
		snprintf(out, outlen, "%uM", (unsigned)(v / (1024 * 1024)));
	else if ((v % 1024) == 0)
		snprintf(out, outlen, "%uK", (unsigned)(v / 1024));
	else
		snprintf(out, outlen, "%u", (unsigned)v);
}

// SPI NOR flash manufacturer ID (JEDEC byte 0) -> human readable name.
static const char *flash_vendor(uint8_t mid) {
	switch (mid) {
	case 0xC8: return "GigaDevice";
	case 0xEF: return "Winbond";
	case 0xC2: return "Macronix";
	case 0x85: return "Puya";
	case 0x20: return "Micron/XMC";
	case 0x1C: return "EON";
	case 0x9D: return "ISSI";
	case 0x0B: return "XTX";
	case 0x68: return "Boya";
	case 0x5E: return "Zbit";
	case 0x51: return "GigaDevice/Elite";
	default:   return "unknown";
	}
}

static const char *type_hint(uint8_t type) {
	switch (type) {
	case 0:  return "FW";
	case 2:  return "mfg/media";
	case 3:  return "media/PSM";
	case 4:  return "PSM/KEY";
	case 5:  return "KEY/DATA";
	case 6:  return "DATA/factory";
	case 7:  return "factory";
	case 10: return "mfg";
	case 16: return "Boot2";
	default: return "?";
	}
}

// Read and validate one partition table located at 'offset' in flash.
static void pt_parse(uint32_t offset, pt_table_t *t) {
	uint8_t buf[PT_HEADER_SIZE + PT_ENTRY_SIZE * PT_ENTRY_MAX + 4];
	int i, entriesLen;
	const uint8_t *e;

	memset(t, 0, sizeof(*t));
	t->offset = offset;
	t->error = "";

	if (HAL_FlashRead((char *)buf, sizeof(buf), offset) != 0) {
		t->error = "flash read failed";
		return;
	}

	t->magic = rd32(buf);
	t->version = rd16(buf + 4);
	t->entryCnt = rd16(buf + 6);
	t->age = rd32(buf + 8);
	t->headerCrc = rd32(buf + 12);

	if (t->magic != PT_MAGIC) {
		t->error = "bad magic";
		return;
	}
	if (pt_crc32(buf, 12) != t->headerCrc) {
		t->error = "bad header crc";
		return;
	}
	if (t->entryCnt > PT_ENTRY_MAX) {
		t->error = "too many entries";
		return;
	}

	entriesLen = t->entryCnt * PT_ENTRY_SIZE;
	t->entriesCrc = rd32(buf + PT_HEADER_SIZE + entriesLen);
	if (pt_crc32(buf + PT_HEADER_SIZE, entriesLen) != t->entriesCrc) {
		t->error = "bad entries crc";
		return;
	}

	for (i = 0; i < t->entryCnt; i++) {
		e = buf + PT_HEADER_SIZE + i * PT_ENTRY_SIZE;
		t->entries[i].type = e[0];
		t->entries[i].device = e[1];
		t->entries[i].activeIndex = e[2];
		memcpy(t->entries[i].name, e + 3, 9);
		t->entries[i].name[9] = '\0';
		t->entries[i].address[0] = rd32(e + 12);
		t->entries[i].address[1] = rd32(e + 16);
		t->entries[i].size[0] = rd32(e + 20);
		t->entries[i].size[1] = rd32(e + 24);
		t->entries[i].len = rd32(e + 28);
		t->entries[i].age = rd32(e + 32);
	}
	t->valid = 1;
}

static void pt_print_summary(const pt_table_t *t, const pt_table_t *active) {
	if (t->magic == PT_MAGIC) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "%c table@0x%05x: %s version=%u entries=%u age=%u hcrc=0x%08x ecrc=0x%08x",
			(t == active) ? '*' : ' ', (unsigned)t->offset,
			t->valid ? "valid" : t->error,
			t->version, t->entryCnt, (unsigned)t->age,
			(unsigned)t->headerCrc, (unsigned)t->entriesCrc);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "%c table@0x%05x: invalid: %s",
			(t == active) ? '*' : ' ', (unsigned)t->offset, t->error);
	}
}

// Prints all entries of a table, the FW slots and the end of the partitioned
// area, and returns that end address (highest addr+size of any partition).
static uint32_t pt_print_entries(const pt_table_t *t) {
	char s0[12], s1[12];
	const pt_entry_t *fw = NULL;
	uint32_t end = 0;
	int i;

	ADDLOG_INFO(LOG_FEATURE_CMD, "  idx type dev act name       addr0     size0      end0      addr1     size1      end1");
	for (i = 0; i < t->entryCnt; i++) {
		const pt_entry_t *en = &t->entries[i];
		uint32_t end0 = en->size[0] ? en->address[0] + en->size[0] : 0;
		uint32_t end1 = en->size[1] ? en->address[1] + en->size[1] : 0;
		const char *name = en->name[0] ? en->name : type_hint(en->type);
		fmt_size(en->size[0], s0, sizeof(s0));
		fmt_size(en->size[1], s1, sizeof(s1));
		ADDLOG_INFO(LOG_FEATURE_CMD, "  %2d  %3u  %2u  %2u %-9s 0x%06x %8s 0x%06x 0x%06x %8s 0x%06x",
			i, en->type, en->device, en->activeIndex, name,
			(unsigned)en->address[0], s0, (unsigned)end0,
			(unsigned)en->address[1], s1, (unsigned)end1);

		if (en->size[0]) {
			uint32_t e0 = en->address[0] + en->size[0];
			if (e0 > end) end = e0;
		}
		if (en->size[1]) {
			uint32_t e1 = en->address[1] + en->size[1];
			if (e1 > end) end = e1;
		}
		if (fw == NULL && (strcmp(en->name, "FW") == 0 || en->type == 0))
			fw = en;
	}

	if (fw) {
		int slot = (fw->activeIndex == 0 || fw->activeIndex == 1) ? fw->activeIndex : 0;
		int other = 1 - slot;
		fmt_size(fw->size[slot], s0, sizeof(s0));
		ADDLOG_INFO(LOG_FEATURE_CMD, "  FW active slot: %d @ 0x%06x size=%s",
			slot, (unsigned)fw->address[slot], s0);
		fmt_size(fw->size[other], s1, sizeof(s1));
		ADDLOG_INFO(LOG_FEATURE_CMD, "  FW other/OTA slot: %d @ 0x%06x size=%s",
			other, (unsigned)fw->address[other], s1);
	}

	fmt_size(end, s0, sizeof(s0));
	ADDLOG_INFO(LOG_FEATURE_CMD, "  partitioned end: 0x%06x (%s)", (unsigned)end, s0);
	return end;
}

// BL602PartitionInfo - dump the BL602 partition tables.
static commandResult_t CMD_BL602PartitionInfo(const void *context, const char *cmd, const char *args, int cmdFlags) {
	pt_table_t tables[2];
	const pt_table_t *active = NULL;
	uint8_t jedec[3];
	uint32_t chipSize = 0;
	char s0[12];
	int i;

	pt_parse(PT_TABLE0_ADDRESS, &tables[0]);
	pt_parse(PT_TABLE1_ADDRESS, &tables[1]);

	// Physical flash chip info from the JEDEC ID (3 bytes):
	//   byte0 = manufacturer, byte1 = memory type, byte2 = capacity (1<<byte2).
	pt_read_jedec_id(jedec);
	ADDLOG_INFO(LOG_FEATURE_CMD, "flash JEDEC ID: 0x%02x%02x%02x (%s)",
		jedec[0], jedec[1], jedec[2], flash_vendor(jedec[0]));
	if (jedec[2] >= 0x10 && jedec[2] <= 0x1f) {
		chipSize = 1u << jedec[2];
		fmt_size(chipSize, s0, sizeof(s0));
		ADDLOG_INFO(LOG_FEATURE_CMD, "total flash size: %s (0x%08x)", s0, (unsigned)chipSize);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "total flash size: unknown (capacity byte 0x%02x)", jedec[2]);
	}

	// Pick active table: highest age wins; on a tie table0 wins (matches BouffaloSDK partition.c).
	for (i = 0; i < 2; i++) {
		if (!tables[i].valid)
			continue;
		if (active == NULL || tables[i].age > active->age)
			active = &tables[i];
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "== BL602 partition table (live flash) ==");
	pt_print_summary(&tables[0], active);
	pt_print_summary(&tables[1], active);

	if (active == NULL) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "  no valid partition table found");
		return CMD_RES_OK;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "  active table: 0x%05x", (unsigned)active->offset);
	{
		uint32_t end = pt_print_entries(active);
		// Anything between the partition table's end and the physical chip end
		// is not described by the partition table (unused / unpartitioned).
		if (chipSize > end) {
			fmt_size(chipSize - end, s0, sizeof(s0));
			ADDLOG_INFO(LOG_FEATURE_CMD, "  unpartitioned: 0x%06x..0x%06x (%s) - not in partition table",
				(unsigned)end, (unsigned)chipSize, s0);
		} else if (chipSize && chipSize == end) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "  whole chip is partitioned");
		}
	}
	return CMD_RES_OK;
}

// ---------------------------------------------------------------------------
// BL602RepartitionPSM: grow the (tiny) PSM partition by merging the unused
// adjacent 'media' partition into it, so EasyFlash has room for the OBK config.
//
// Why: factory 1MB layouts (e.g. CosyLife) ship PSM = 8KB. EasyFlash reserves a
// full 4KB sector for GC, leaving ~4KB usable - too small for the ~3.6KB OBK
// config blob plus the SDK's own ENV entries, so WiFi config can never be saved
// ("ENV full"). 'media' (type 3) sits immediately before PSM and is unused by
// OpenBeken, so we hand its space to PSM. This is doable over OTA (no serial).
//
// Safety: only one of the two table copies (0xE000 / 0xF000) is ever rewritten
// at a time and it is read back + CRC-verified before touching the other, so a
// power loss mid-write always leaves at least one valid table for boot2 to use.
// We refuse unless 'media' is exactly adjacent below PSM. EasyFlash data and the
// old config are erased so the bigger region initialises clean on next boot.
// ---------------------------------------------------------------------------

// On-flash partition structures, matching the BouffaloSDK layout exactly (no
// padding). PtTable_Update_Entry (std driver) recomputes both CRCs and writes
// one table copy using the flash ops already installed at boot.
typedef struct {
	uint32_t magicCode;
	uint16_t version;
	uint16_t entryCnt;
	uint32_t age;
	uint32_t crc32;
} sdkPtCfg_t;

typedef struct {
	uint8_t  type;
	uint8_t  device;
	uint8_t  activeIndex;
	uint8_t  name[9];
	uint32_t Address[2];
	uint32_t maxLen[2];
	uint32_t len;
	uint32_t age;
} sdkPtEntry_t;

typedef struct {
	sdkPtCfg_t   ptTable;
	sdkPtEntry_t ptEntries[PT_ENTRY_MAX];
	uint32_t     crc32;
} sdkPtStuff_t;

// NOTE: the linked PtTable_Update_Entry is the hosal (bl_boot2.c) variant with
// FOUR args: (pFlashCfg, targetTableID, ptStuff, ptEntry). pFlashCfg is unused
// by the implementation (it writes via bl_flash_erase/bl_flash_write), so we
// pass NULL exactly like hal_boot2.c does. targetTableID: 0 = copy@0xE000,
// 1 = copy@0xF000. Returns 0 (PT_ERROR_SUCCESS) on success.
extern int PtTable_Update_Entry(const void *pFlashCfg, int targetTableID, void *ptStuff, void *ptEntry);
extern int bl_flash_erase(uint32_t addr, int len);

// PSM sizing targets for the repartition command.
#define PSM_IDEAL   0x4000   // 16K - grow up to this when room allows
#define PSM_MIN     0x3000   // 12K - minimum we accept (~8K usable in EasyFlash)
#define PSM_CAP     0x8000   // 32K - never grow beyond this (limits the erase)
#define MEDIA_MIN   0x1000   // shrink 'media' to one 4K sector (it only needs to exist)

// Read one table copy from flash into a sdkPtStuff_t, validating magic + both
// CRCs. Returns the entry count, or -1 if invalid.
static int pt_read_stuff(uint32_t addr, sdkPtStuff_t *stuff) {
	uint8_t buf[PT_HEADER_SIZE + PT_ENTRY_SIZE * PT_ENTRY_MAX + 4];
	uint16_t cnt;
	if (HAL_FlashRead((char *)buf, sizeof(buf), addr) != 0)
		return -1;
	if (rd32(buf) != PT_MAGIC)
		return -1;
	if (pt_crc32(buf, 12) != rd32(buf + 12))
		return -1;
	cnt = rd16(buf + 6);
	if (cnt > PT_ENTRY_MAX)
		return -1;
	if (pt_crc32(buf + PT_HEADER_SIZE, cnt * PT_ENTRY_SIZE) != rd32(buf + PT_HEADER_SIZE + cnt * PT_ENTRY_SIZE))
		return -1;
	memset(stuff, 0, sizeof(*stuff));
	memcpy(&stuff->ptTable, buf, PT_HEADER_SIZE);
	memcpy(stuff->ptEntries, buf + PT_HEADER_SIZE, cnt * PT_ENTRY_SIZE);
	return cnt;
}

static int pt_find_entry(const sdkPtStuff_t *stuff, int cnt, const char *name) {
	char nm[10];
	int i;
	for (i = 0; i < cnt; i++) {
		memcpy(nm, stuff->ptEntries[i].name, 9);
		nm[9] = '\0';
		if (strcmp(nm, name) == 0)
			return i;
	}
	return -1;
}

// Read back a table copy and confirm its PSM entry has the expected addr/size
// (also re-validates CRC via pt_read_stuff). Returns 1 on match.
static int pt_verify_psm(uint32_t tableAddr, uint32_t expAddr, uint32_t expSize) {
	sdkPtStuff_t s;
	int cnt, idx;
	cnt = pt_read_stuff(tableAddr, &s);
	if (cnt < 0)
		return 0;
	idx = pt_find_entry(&s, cnt, "PSM");
	if (idx < 0)
		return 0;
	return (s.ptEntries[idx].Address[0] == expAddr && s.ptEntries[idx].maxLen[0] == expSize);
}

static commandResult_t CMD_BL602RepartitionPSM(const void *context, const char *cmd, const char *args, int cmdFlags) {
	pt_table_t t0, t1;
	const pt_table_t *active = NULL;
	sdkPtStuff_t stuff;
	int cnt, psmIdx, mediaIdx, confirm, rc, i, useMedia;
	uint32_t psmAddr, psmLen, newAddr, newSize, ceiling, gapGrow, mediaReclaim, mediaAddr, mediaLen;
	char s0[12];

	Tokenizer_TokenizeString(args, 0);
	confirm = (Tokenizer_GetArgsCount() > 0 && strcmp(Tokenizer_GetArg(0), "CONFIRM") == 0);

	pt_parse(PT_TABLE0_ADDRESS, &t0);
	pt_parse(PT_TABLE1_ADDRESS, &t1);
	if (t0.valid && (!t1.valid || t0.age >= t1.age))
		active = &t0;
	else if (t1.valid)
		active = &t1;
	if (active == NULL) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "RepartitionPSM: no valid partition table - refusing");
		return CMD_RES_ERROR;
	}

	cnt = pt_read_stuff(active->offset, &stuff);
	if (cnt < 0) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "RepartitionPSM: failed to read/validate active table @0x%05x", (unsigned)active->offset);
		return CMD_RES_ERROR;
	}

	psmIdx = pt_find_entry(&stuff, cnt, "PSM");
	if (psmIdx < 0) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "RepartitionPSM: no 'PSM' entry - refusing");
		return CMD_RES_ERROR;
	}
	psmAddr = stuff.ptEntries[psmIdx].Address[0];
	psmLen = stuff.ptEntries[psmIdx].maxLen[0];

	if (psmLen >= PSM_IDEAL) {
		fmt_size(psmLen, s0, sizeof(s0));
		ADDLOG_INFO(LOG_FEATURE_CMD, "RepartitionPSM: PSM already %s - nothing to do", s0);
		return CMD_RES_OK;
	}

	// Ceiling = start of the partition immediately above PSM; PSM may grow up to
	// there into any unallocated gap.
	ceiling = 0xFFFFFFFF;
	for (i = 0; i < cnt; i++) {
		uint32_t a = stuff.ptEntries[i].Address[0];
		if (stuff.ptEntries[i].maxLen[0] == 0)
			continue;
		if (a > psmAddr && a < ceiling)
			ceiling = a;
	}
	if (ceiling == 0xFFFFFFFF) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "RepartitionPSM: no partition above PSM - refusing");
		return CMD_RES_ERROR;
	}
	gapGrow = ceiling - psmAddr;   // PSM size if we only grow up into the gap

	// Extra room reclaimable by shrinking an adjacent 'media' (ROMFS) below PSM to
	// one sector. media only needs a non-zero address to avoid the boot dead-loop;
	// its size/contents are irrelevant to OpenBeken (romfs just soft-fails).
	mediaIdx = pt_find_entry(&stuff, cnt, "media");
	mediaAddr = mediaLen = mediaReclaim = 0;
	if (mediaIdx >= 0) {
		mediaAddr = stuff.ptEntries[mediaIdx].Address[0];
		mediaLen = stuff.ptEntries[mediaIdx].maxLen[0];
		if (mediaLen > MEDIA_MIN && (mediaAddr + mediaLen) == psmAddr)
			mediaReclaim = mediaLen - MEDIA_MIN;
	}

	// Pick the method: grow into the gap only (least invasive); shrink media too
	// only if the gap alone can't reach the ideal size.
	if (gapGrow >= PSM_IDEAL) {
		useMedia = 0;
		newAddr = psmAddr;
		newSize = gapGrow > PSM_CAP ? PSM_CAP : gapGrow;
	} else {
		useMedia = (mediaReclaim > 0);
		newAddr = psmAddr - mediaReclaim;
		newSize = gapGrow + mediaReclaim;
		if (newSize > PSM_CAP)
			newSize = PSM_CAP;
	}

	if (newSize < PSM_MIN || newSize <= psmLen) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "RepartitionPSM: not enough free flash around PSM (gap=0x%x, media-reclaim=0x%x, max=0x%x, need>=0x%x) - refusing",
			(unsigned)gapGrow, (unsigned)mediaReclaim, (unsigned)(gapGrow + mediaReclaim), (unsigned)PSM_MIN);
		return CMD_RES_ERROR;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "=== BL602 RepartitionPSM ===");
	fmt_size(psmLen, s0, sizeof(s0));
	ADDLOG_INFO(LOG_FEATURE_CMD, "  current PSM: 0x%06x size=0x%x (%s)", (unsigned)psmAddr, (unsigned)psmLen, s0);
	fmt_size(newSize, s0, sizeof(s0));
	ADDLOG_INFO(LOG_FEATURE_CMD, "  -> new PSM:  0x%06x size=0x%x (%s)", (unsigned)newAddr, (unsigned)newSize, s0);
	if (useMedia) {
		fmt_size(MEDIA_MIN, s0, sizeof(s0));
		ADDLOG_INFO(LOG_FEATURE_CMD, "  method: gap + shrink 'media' @0x%06x to %s (next part @0x%06x)", (unsigned)mediaAddr, s0, (unsigned)ceiling);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "  method: grow into free gap, 'media' untouched (next part @0x%06x)", (unsigned)ceiling);
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "  active table @0x%05x age=%u; will rewrite both copies", (unsigned)active->offset, (unsigned)stuff.ptTable.age);

	if (!confirm) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "  DRY-RUN only. To apply (ERASES config + reboots): BL602RepartitionPSM CONFIRM");
		return CMD_RES_OK;
	}

	// PSM takes the new address+size. When shrinking, media keeps its (non-zero)
	// address but drops to MEDIA_MIN. PtTable_Update_Entry recomputes both CRCs.
	stuff.ptEntries[psmIdx].Address[0] = newAddr;
	stuff.ptEntries[psmIdx].maxLen[0] = newSize;
	if (useMedia)
		stuff.ptEntries[mediaIdx].maxLen[0] = MEDIA_MIN;

	// Copy 1 (0xF000) first - copy 0 stays valid (old layout) as a fallback.
	ADDLOG_INFO(LOG_FEATURE_CMD, "  writing table copy 1 (0xF000)...");
	rc = PtTable_Update_Entry(NULL, 1, &stuff, &stuff.ptEntries[psmIdx]);
	if (rc != 0) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "  copy 1 write returned %d - aborted. Table 0 intact, device still boots (old PSM).", rc);
		return CMD_RES_ERROR;
	}
	if (!pt_verify_psm(PT_TABLE1_ADDRESS, newAddr, newSize)) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "  copy 1 readback verify FAILED - aborted. Table 0 intact, device still boots (old PSM).");
		return CMD_RES_ERROR;
	}
	// Copy 0 (0xE000). If this fails copy 1 is already valid+new, so we proceed.
	ADDLOG_INFO(LOG_FEATURE_CMD, "  copy 1 verified. Writing table copy 0 (0xE000)...");
	rc = PtTable_Update_Entry(NULL, 0, &stuff, &stuff.ptEntries[psmIdx]);
	if (rc != 0 || !pt_verify_psm(PT_TABLE0_ADDRESS, newAddr, newSize)) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "  copy 0 write/verify failed (rc=%d) - but copy 1 holds the new layout, continuing.", rc);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "  copy 0 verified.");
	}

	// Erase the enlarged PSM region so EasyFlash initialises clean next boot
	// (wipes old media bytes + the unsavable old config). factory is at the next
	// sector and is NOT touched.
	ADDLOG_INFO(LOG_FEATURE_CMD, "  erasing new PSM region 0x%06x len 0x%x...", (unsigned)newAddr, (unsigned)newSize);
	bl_flash_erase(newAddr, newSize);

	ADDLOG_INFO(LOG_FEATURE_CMD, "  DONE. Rebooting in 3s into new layout. Reconfigure WiFi after boot.");
	RESET_ScheduleModuleReset(3);
	return CMD_RES_OK;
}

void BL602_AddPartitionInfoCommand(void) {
	//cmddetail:{"name":"BL602PartitionInfo","args":"",
	//cmddetail:"descr":"BL602: dumps the two Bouffalo partition tables read live from flash (0xE000/0xF000), showing which one is active, the active/OTA firmware slots, partition sizes and where the partitioned area ends.",
	//cmddetail:"fn":"CMD_BL602PartitionInfo","file":"hal/bl602/hal_partition_bl602.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BL602PartitionInfo", CMD_BL602PartitionInfo, NULL);
	//cmddetail:{"name":"BL602RepartitionPSM","args":"[CONFIRM]",
	//cmddetail:"descr":"BL602: enlarges the tiny PSM partition (EasyFlash/config storage) so WiFi/config can be saved on 1MB-style layouts with an 8KB PSM. It auto-picks a method: first grows PSM into the unallocated flash gap after it (least invasive, 'media' untouched); if the gap alone is too small it also shrinks the adjacent unused 'media' (ROMFS) partition down to one sector to free more room. Refuses if neither yields a viable size. Without CONFIRM it only prints the plan (dry-run); with CONFIRM it rewrites both partition table copies, erases the old config and reboots.",
	//cmddetail:"fn":"CMD_BL602RepartitionPSM","file":"hal/bl602/hal_partition_bl602.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BL602RepartitionPSM", CMD_BL602RepartitionPSM, NULL);
}

#endif // PLATFORM_BL602 && !PLATFORM_BL_NEW
