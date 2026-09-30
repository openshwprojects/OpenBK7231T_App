// Firmware update of a TuyaMCU (the external MCU driven by the TuyaMCU driver)
// over the standard Tuya MCU upgrade protocol:
//
//   module -> MCU  0x0A  [image size, u32 BE]            upgrade start
//   MCU -> module  0x0A  [packet size: 0=256, 1=512, 2=1024 bytes]
//   module -> MCU  0x0B  [offset, u32 BE][data]          one per packet
//   MCU -> module  0x0B  []                              ack
//   module -> MCU  0x0B  [offset = image size]           end (no data)
//
// The MCU then checks the image and installs it, usually by resetting. A
// heartbeat reply of 0 afterwards (first heartbeat since MCU start) is taken
// as confirmation.
//
// The image is read from LittleFS; upload it first, e.g.
//   curl --data-binary @mcu.bin http://<ip>/api/lfs/mcu.bin
// then run
//   tuyaMcu_otaStart mcu.bin

#include "../new_common.h"
#include "../obk_config.h"

#if ENABLE_TUYAMCU_OTA

#include "../new_cfg.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../littlefs/our_lfs.h"
#include "drv_public.h"
#include "drv_tuyaMCU.h"
#include "drv_tuyaMCU_ota.h"

#define TUYA_CMD_HEARTBEAT         0x00
#define TUYA_CMD_UPGRADE_START     0x0A
#define TUYA_CMD_UPGRADE_DATA      0x0B

// The MCU may erase its whole update area before answering
#define OTA_START_TIMEOUT_MS       10000
// A 1 KB packet takes ~1.1 s at 9600 baud; the MCU then writes it to flash
#define OTA_ACK_TIMEOUT_MS         3000
#define OTA_MAX_RETRIES            3
#define OTA_REBOOT_TIMEOUT_MS      20000
#define OTA_MAX_PACKET_SIZE        1024

typedef enum {
	OTA_IDLE,
	OTA_WAIT_START,     // 0x0A sent, waiting for the packet size
	OTA_SEND,           // next packet due
	OTA_WAIT_ACK,       // packet sent, waiting for the 0x0B ack
	OTA_WAIT_REBOOT,    // all data sent, waiting for the MCU to restart
	OTA_DONE,
	OTA_FAILED,
} tuyaOtaState_t;

static const char *g_stateNames[] = {
	"idle", "starting", "sending", "sending", "waiting for MCU restart", "done", "failed"
};

static tuyaOtaState_t g_state = OTA_IDLE;
static lfs_file_t g_file;
static bool g_fileOpen = false;
static byte *g_buffer = NULL;       // 4-byte offset + one packet
static char g_fileName[64];
static int g_size;                  // image size
static int g_offset;                // bytes acknowledged by the MCU
static int g_inFlight;              // data bytes in the packet awaiting an ack
static int g_packetSize;
static int g_retries;
static int g_timerMS;
static char g_message[96];

static void put_u32_be(byte *p, uint32_t v) {
	p[0] = (byte)(v >> 24);
	p[1] = (byte)(v >> 16);
	p[2] = (byte)(v >> 8);
	p[3] = (byte)v;
}

static void OTA_Release() {
	if (g_fileOpen) {
		lfs_file_close(&lfs, &g_file);
		g_fileOpen = false;
	}
	if (g_buffer) {
		free(g_buffer);
		g_buffer = NULL;
	}
}

static void OTA_Finish(tuyaOtaState_t state, const char *message) {
	OTA_Release();
	g_state = state;
	strcpy_safe(g_message, message, sizeof(g_message));
	addLogAdv(state == OTA_DONE ? LOG_INFO : LOG_ERROR, LOG_FEATURE_TUYAMCU,
		"MCU OTA %s: %s (%i/%i bytes)", g_stateNames[state], message, g_offset, g_size);
}

bool TuyaMCU_OTA_IsActive() {
	return g_state != OTA_IDLE && g_state != OTA_DONE && g_state != OTA_FAILED;
}

const char *TuyaMCU_OTA_GetStateName() {
	return g_stateNames[g_state];
}

static void OTA_SendStart() {
	byte payload[4];
	put_u32_be(payload, g_size);
	TuyaMCU_SendCommandWithData(TUYA_CMD_UPGRADE_START, payload, sizeof(payload));
	g_timerMS = OTA_START_TIMEOUT_MS;
	g_state = OTA_WAIT_START;
}

static void OTA_SendPacket() {
	int n = g_size - g_offset;
	if (n > g_packetSize) {
		n = g_packetSize;
	}
	if (lfs_file_seek(&lfs, &g_file, g_offset, LFS_SEEK_SET) < 0 ||
		lfs_file_read(&lfs, &g_file, g_buffer + 4, n) != n) {
		OTA_Finish(OTA_FAILED, "cannot read the image file");
		return;
	}
	put_u32_be(g_buffer, g_offset);
	TuyaMCU_SendCommandWithData(TUYA_CMD_UPGRADE_DATA, g_buffer, 4 + n);
	g_inFlight = n;
	g_timerMS = OTA_ACK_TIMEOUT_MS;
	g_state = OTA_WAIT_ACK;
}

// Standard end marker: offset = image size, no data. Not all MCUs answer it
// (some reset as soon as the last byte arrives), so no ack is awaited.
static void OTA_SendEnd() {
	byte payload[4];
	put_u32_be(payload, g_size);
	TuyaMCU_SendCommandWithData(TUYA_CMD_UPGRADE_DATA, payload, sizeof(payload));
	g_timerMS = OTA_REBOOT_TIMEOUT_MS;
	g_state = OTA_WAIT_REBOOT;
}

bool TuyaMCU_OTA_OnPacket(byte cmd, const byte *payload, int len) {
	if (!TuyaMCU_OTA_IsActive()) {
		return false;
	}
	switch (cmd) {
	case TUYA_CMD_UPGRADE_START:
		if (g_state == OTA_WAIT_START) {
			int code = len >= 1 ? payload[0] : 0;
			if (code > 2) {
				OTA_Finish(OTA_FAILED, "MCU asked for an unsupported packet size");
				return true;
			}
			g_packetSize = 256 << code;
			g_buffer = (byte *)malloc(4 + g_packetSize);
			if (g_buffer == NULL) {
				OTA_Finish(OTA_FAILED, "out of memory");
				return true;
			}
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "MCU OTA: MCU accepted %i bytes, %i-byte packets",
				g_size, g_packetSize);
			g_offset = 0;
			g_retries = 0;
			g_state = OTA_SEND;
		}
		return true;

	case TUYA_CMD_UPGRADE_DATA:
		if (g_state == OTA_WAIT_ACK) {
			g_offset += g_inFlight;
			g_inFlight = 0;
			g_retries = 0;
			if (g_offset >= g_size) {
				addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "MCU OTA: all %i bytes sent", g_size);
				OTA_SendEnd();
			}
			else {
				g_state = OTA_SEND;
			}
		}
		return true;

	case TUYA_CMD_HEARTBEAT:
		// 0 = first heartbeat since the MCU started: it has rebooted
		if (g_state == OTA_WAIT_REBOOT && len >= 1 && payload[0] == 0) {
			OTA_Finish(OTA_DONE, "MCU restarted after the update");
		}
		return false;

	default:
		return false;
	}
}

void TuyaMCU_OTA_RunFrame() {
	switch (g_state) {
	case OTA_WAIT_START:
		g_timerMS -= (int)g_deltaTimeMS;
		if (g_timerMS <= 0) {
			OTA_Finish(OTA_FAILED, "MCU did not answer the upgrade start (0x0A)");
		}
		break;

	case OTA_SEND:
		OTA_SendPacket();
		break;

	case OTA_WAIT_ACK:
		g_timerMS -= (int)g_deltaTimeMS;
		if (g_timerMS <= 0) {
			if (++g_retries > OTA_MAX_RETRIES) {
				OTA_Finish(OTA_FAILED, "MCU stopped acknowledging packets");
			}
			else {
				addLogAdv(LOG_WARN, LOG_FEATURE_TUYAMCU, "MCU OTA: no ack at offset %i, retry %i",
					g_offset, g_retries);
				OTA_SendPacket();
			}
		}
		break;

	case OTA_WAIT_REBOOT:
		g_timerMS -= (int)g_deltaTimeMS;
		if (g_timerMS <= 0) {
			OTA_Finish(OTA_FAILED, "all data sent, but the MCU did not restart (image rejected?)");
		}
		break;

	default:
		break;
	}
}

// tuyaMcu_otaStart [fileName]
static commandResult_t CMD_TuyaMCU_OTA_Start(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *fname;
	int lfsres;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	if (TuyaMCU_OTA_IsActive()) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "MCU OTA: an update is already running");
		return CMD_RES_ERROR;
	}
	if (!lfs_present()) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "MCU OTA: LittleFS is not available");
		return CMD_RES_ERROR;
	}

	fname = Tokenizer_GetArg(0);
	memset(&g_file, 0, sizeof(g_file));
	lfsres = lfs_file_open(&lfs, &g_file, fname, LFS_O_RDONLY);
	if (lfsres < 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "MCU OTA: cannot open %s (%i)", fname, lfsres);
		return CMD_RES_ERROR;
	}
	g_fileOpen = true;
	g_size = lfs_file_size(&lfs, &g_file);
	if (g_size <= 0) {
		OTA_Release();
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "MCU OTA: %s is empty", fname);
		return CMD_RES_ERROR;
	}

	strcpy_safe(g_fileName, fname, sizeof(g_fileName));
	g_offset = 0;
	g_inFlight = 0;
	g_packetSize = 0;
	g_retries = 0;
	g_message[0] = 0;
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "MCU OTA: sending %s (%i bytes)", g_fileName, g_size);
	OTA_SendStart();
	return CMD_RES_OK;
}

// tuyaMcu_otaAbort
static commandResult_t CMD_TuyaMCU_OTA_Abort(const void *context, const char *cmd, const char *args, int cmdFlags) {
	if (!TuyaMCU_OTA_IsActive()) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "MCU OTA: no update is running");
		return CMD_RES_OK;
	}
	// The MCU discards a partial update when the data stops
	OTA_Finish(OTA_FAILED, "aborted");
	return CMD_RES_OK;
}

// tuyaMcu_otaStatus
static commandResult_t CMD_TuyaMCU_OTA_Status(const void *context, const char *cmd, const char *args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "MCU OTA: %s, %s, %i/%i bytes%s%s",
		g_stateNames[g_state], g_state == OTA_IDLE ? "-" : g_fileName, g_offset, g_size,
		g_message[0] ? ", " : "", g_message);
	return CMD_RES_OK;
}

void TuyaMCU_OTA_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState || g_state == OTA_IDLE) {
		return;
	}
	hprintf255(request, "<h5>MCU update: %s, %i/%i bytes (%i%%)%s%s</h5>",
		g_stateNames[g_state], g_offset, g_size, g_size ? (int)((int64_t)g_offset * 100 / g_size) : 0,
		g_message[0] ? " - " : "", g_message);
}

void TuyaMCU_OTA_Init() {
	g_state = OTA_IDLE;

	//cmddetail:{"name":"tuyaMcu_otaStart","args":"[FileName]",
	//cmddetail:"descr":"Updates the TuyaMCU's own firmware with an image stored in LittleFS, using the Tuya MCU upgrade commands (0x0A/0x0B). Upload the image first, for example with curl --data-binary @mcu.bin http://IP/api/lfs/mcu.bin. Only for MCUs that implement the Tuya MCU upgrade protocol; progress is shown on the main page and by tuyaMcu_otaStatus.",
	//cmddetail:"fn":"CMD_TuyaMCU_OTA_Start","file":"driver/drv_tuyaMCU_ota.c","requires":"",
	//cmddetail:"examples":"tuyaMcu_otaStart mcu.bin"}
	CMD_RegisterCommand("tuyaMcu_otaStart", CMD_TuyaMCU_OTA_Start, NULL);
	//cmddetail:{"name":"tuyaMcu_otaAbort","args":"",
	//cmddetail:"descr":"Stops a running TuyaMCU firmware update. The MCU discards the partial image.",
	//cmddetail:"fn":"CMD_TuyaMCU_OTA_Abort","file":"driver/drv_tuyaMCU_ota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_otaAbort", CMD_TuyaMCU_OTA_Abort, NULL);
	//cmddetail:{"name":"tuyaMcu_otaStatus","args":"",
	//cmddetail:"descr":"Logs the state and progress of the TuyaMCU firmware update.",
	//cmddetail:"fn":"CMD_TuyaMCU_OTA_Status","file":"driver/drv_tuyaMCU_ota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("tuyaMcu_otaStatus", CMD_TuyaMCU_OTA_Status, NULL);
}

void TuyaMCU_OTA_Shutdown() {
	OTA_Release();
	g_state = OTA_IDLE;
}

#endif // ENABLE_TUYAMCU_OTA
