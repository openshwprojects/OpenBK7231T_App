// LiftMaster / Chamberlain commercial-operator host-link driver (Saturn / msg1210).
// See drv_liftmaster.h for the wire format and provenance.

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_public.h"
#include "drv_liftmaster.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../httpserver/hass.h"
#include "../mqtt/new_mqtt.h"

// ---------------------------------------------------------------- config -----
#define LM_BAUD        57600
#define LM_PARITY      0        // 8N1 (0=none, 1=odd, 2=even in UART_InitUART)
#define LM_RX_RING     1024
#define LM_FRAME_MAX   256      // max on-wire frame length we handle
#define LM_PAYLOAD_MAX 255

// Door status frame (RX, LPC->us): header 01 11 02 11, payload length 16,
// direction = last payload byte: 0x01=OPEN, 0x00=CLOSED. (docs/lpc_protocol.md)
static const byte LM_STATUS_HDR[4] = { 0x01, 0x11, 0x02, 0x11 };
#define LM_STATUS_LEN 16

// OBK channel that receives the decoded door state (0=closed, 1=open).
static int g_statusChannel = 1;

// --- ARQ (stop-and-wait) transport state --------------------------------------
// The Saturn link uses a reliable, sequenced transport. CRITICAL: the operator's
// data sequence counter is MODULO 8 (verified in the LPC firmware, the TX builder
// does `seq = (seq+1) & 7`). Sending a seq outside 0..7 leaves its receive window
// and the operator NAKs, desyncing the link (observed: ~7 good frames then a NAK
// stall). So our TX seq is mod-8, and we run a real stop-and-wait ARQ: buffer the
// last data frame, (re)send until the operator ACKs it (`<K{seq}>`), retransmit
// the SAME frame on NAK (`<N..>`) or timeout, advance the seq only once ACKed.
#define LM_SEQ_MOD      8       // operator data-seq is 3-bit (mod 8)
#define LM_TX_MAX_TRIES 6       // retransmit budget per data frame
#define LM_TX_TMO_TICKS 1       // RunEverySecond ticks before a timeout resend
static byte g_txSeq = 0;        // next seq to use for a NEW data frame (mod 8)

// the single outstanding (unacknowledged) data frame we own the ARQ for
static byte g_pendFrame[LM_FRAME_MAX];
static int  g_pendLen = 0;      // 0 = nothing outstanding
static byte g_pendSeq = 0;      // seq carried by g_pendFrame
static int  g_pendTries = 0;    // remaining retransmits
static int  g_pendTicks = 0;    // RunEverySecond ticks since last (re)send

// stats
static uint32_t g_rxFrames = 0, g_rxCrcErr = 0, g_txFrames = 0, g_txAcks = 0;
static uint32_t g_rxAcks = 0, g_rxNaks = 0, g_txResends = 0, g_txDropped = 0;

// Last frame seen in each direction, kept as printable text for the web UI.
// An installed board has no serial console, so the index page is our only
// window onto the link -- see LiftMaster_AppendInformationToHTTPIndexPage.
#define LM_LAST_STR 72
static char g_lastRxStr[LM_LAST_STR] = "(none)";
static char g_lastTxStr[LM_LAST_STR] = "(none)";

// Ring of recent RX frames. Polling the index page over WiFi runs at a few Hz,
// which is far slower than the operator emits frames during a door cycle, so a
// single "last frame" slot silently drops the interesting transitions. Keep the
// last LM_RING_N so one poll can recover a whole actuation sequence.
#define LM_RING_N 16
static char g_ring[LM_RING_N][LM_LAST_STR];
static byte g_ringHead = 0;             // next slot to write
static uint32_t g_ringTotal = 0;        // frames ever stored (ring may have wrapped)

static void LM_NoteFrame(char *dst, const byte *frame, int len) {
	int i, n = 0;
	for (i = 0; i < len && n < LM_LAST_STR - 2; i++) {
		byte b = frame[i];
		dst[n++] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
	}
	dst[n] = 0;
}

// last door direction reported by the operator (-1 unknown, 0 closed, 1 open).
// Only tracks the two SETTLED states, so "is it shut?" logic stays simple.
static int g_lastDir = -1;
// full door state as reported by the operator's 0x0D TLV; -1 until first frame.
// 0 = CLOSED, 1 = OPEN, 2 = MOVING, 3 = STOPPED part-way.
static int g_doorState = -1;
// desired door direction (for channel sync / logging); -1 = none
static int g_doorTarget = -1;
// guard so our own RX-driven CHANNEL_Set doesn't re-trigger a command
static int g_suppressChannelCb = 0;

// published on the door channel when the real position is not yet known (matches
// neither state_open nor state_closed, so HA shows the cover as unknown)
#define LM_STATE_UNKNOWN 9

static const char *LM_StateName(int st) {
	switch (st) {
		case 0:  return "CLOSED";
		case 1:  return "OPEN";
		case 2:  return "MOVING";
		case 3:  return "STOPPED";
		default: return "UNKNOWN";
	}
}

static byte g_crcTable[256];

static void LM_SetupDoorSwitchUI(void); // web-UI switch on the door-state channel

// ------------------------------------------------------------------ crc ------
static void LM_BuildCrcTable(void) {
	int i, b;
	for (i = 0; i < 256; i++) {
		int c = i;
		for (b = 0; b < 8; b++)
			c = (c & 0x80) ? (((c << 1) ^ 0x1D) & 0xFF) : ((c << 1) & 0xFF);
		g_crcTable[i] = (byte)c;
	}
}

static byte LM_Crc8(const byte *data, int len) {
	byte c = 0xAA;
	int i;
	for (i = 0; i < len; i++)
		c = g_crcTable[data[i] ^ c];
	return c;
}

// ------------------------------------------------------------- hex helpers ---
static const char LM_HEX[] = "0123456789ABCDEF";

static int LM_HexNibble(char ch) {
	if (ch >= '0' && ch <= '9') return ch - '0';
	if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
	if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
	return -1;
}

// Parse a null-terminated hex string into bytes. Returns byte count, or -1 on
// a bad/odd-length string or overflow.
static int LM_HexToBytes(const char *s, byte *out, int maxOut) {
	int n = 0;
	while (s[0] && s[1]) {
		int hi = LM_HexNibble(s[0]);
		int lo = LM_HexNibble(s[1]);
		if (hi < 0 || lo < 0 || n >= maxOut) return -1;
		out[n++] = (byte)((hi << 4) | lo);
		s += 2;
	}
	if (s[0]) return -1; // odd number of nibbles
	return n;
}

// ----------------------------------------------------------- frame builder ---
// Build '<' TYPE SEQ hex(hdr[4] + len + payload + crc) '>' into out.
// Returns frame length, or -1 on overflow / bad args.
static int LM_BuildFrame(const byte *hdr, const byte *payload, int plen,
		int seq, char typeChar, byte *out, int maxOut) {
	byte core[5 + LM_PAYLOAD_MAX];
	int clen = 0, o = 0, i;
	byte crc;

	if (plen < 0 || plen > LM_PAYLOAD_MAX) return -1;
	for (i = 0; i < 4; i++) core[clen++] = hdr[i];
	core[clen++] = (byte)plen;
	for (i = 0; i < plen; i++) core[clen++] = payload[i];
	crc = LM_Crc8(core, clen);

	// '<' + TYPE + SEQ + (clen+1 bytes * 2 hex) + '>'
	if (maxOut < 3 + (clen + 1) * 2 + 1) return -1;
	out[o++] = '<';
	out[o++] = (byte)typeChar;
	out[o++] = LM_HEX[seq & 0xF];
	for (i = 0; i < clen; i++) {
		out[o++] = LM_HEX[core[i] >> 4];
		out[o++] = LM_HEX[core[i] & 0xF];
	}
	out[o++] = LM_HEX[crc >> 4];
	out[o++] = LM_HEX[crc & 0xF];
	out[o++] = '>';
	return o;
}

static void LM_SendFrame(const byte *frame, int len) {
	int i;
	for (i = 0; i < len; i++)
		UART_SendByte(frame[i]);
	g_txFrames++;
	LM_NoteFrame(g_lastTxStr, frame, len);
}

// ARQ ACK: the Saturn link is reliable — the peer runs an ack-timer and
// retransmits any data frame we don't acknowledge. Reply to each received data
// frame with '<' 'K' <seqNibble> '>' (the LPC's own FUN_1005318a format) so it
// stops resending. Without this the operator floods stale retransmits and our
// door-state feedback lags/inverts.
static void LM_SendAck(char seqChar) {
	byte f[4];
	f[0] = '<'; f[1] = 'K'; f[2] = (byte)seqChar; f[3] = '>';
	LM_SendFrame(f, 4);
	g_txAcks++;
}

// ------------------------------------------------------- DOOR COMMAND --------
// Msg_S_Motion_Actuate (msg id 0x0280) -- the real door actuation, confirmed
// live: the door both OPENED and CLOSED on these frames.
// (see pwnLiftMaster docs/obk_command_path.md)
//
// TWO things matter, and both were wrong in earlier revisions of this driver:
//
//  1) HDR[2] is the msg1210 CLASS/ROUTE byte and MUST be 0x01.
//     The LPC RX loop does:  class = hdr[2] & 0x7f;
//        class == 1 -> LOCAL dispatch to the endpoint handler   (what we want)
//        else       -> ROUTE the frame out of the port for that class
//     0x02 is the route id of the RTL port itself, so a frame sent with
//     hdr 01 11 02 11 was routed *straight back to us*. That -- not any myQ
//     reply -- is the "echo" this project chased for months, and it meant the
//     message never reached the door logic at all.
//
//  2) The message must be Motion_Actuate (0x0280), not one of the 0x1c-family
//     myQ TLV *status* messages (those only ever report state).
//
// Endpoint 0x11 payload layout (LPC parser FUN_10055f44 -> FUN_10056fa6):
//   payload[0]              = subcmd 0   -> 2.5-byte-payload message path
//   payload[5] low nibble   = 2   \  dispatch key = p[11] | ((p[5]&0xf)<<8)
//   payload[11]             = 0x80 /  => 0x0280 = Motion_Actuate
//   payload[12]             = selector  -> 0x00 CLOSE, 0x01 OPEN (must be < 8)
//   payload[13], payload[14]= flags     -> BOTH MUST BE 0, else silently ignored
//   declared length must be >= 0x0f; we send 16.
//
// Commands are ABSOLUTE STATE, not toggles: CLOSE while already closed (or OPEN
// while already open) correctly does nothing.
//
// No inner/4-bit checksum is needed here -- that one (FUN_10057e6c) belongs to
// the RF / Security+2.0 codec, not this host link. Only the outer CRC-8
// (poly 0x1D, init 0xAA), which LM_BuildFrame already appends, applies.
static const byte LM_DOOR_HDR[4] = { 0x01, 0x11, 0x01, 0x11 };

#define LM_SEL_CLOSE 0x00
#define LM_SEL_OPEN  0x01

static byte g_motionMsg[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x00 /* [12] = selector */, 0x00, 0x00, 0x00
};

// Build a data frame with the next mod-8 seq, buffer it as the single
// outstanding ARQ frame, and transmit it. Any previous un-ACKed frame is
// superseded (door commands coalesce to the latest desired state).
static void LM_SendData(const byte *hdr, const byte *payload, int plen) {
	int flen = LM_BuildFrame(hdr, payload, plen, g_txSeq, 'P',
		g_pendFrame, sizeof(g_pendFrame));
	if (flen <= 0) return;
	g_pendLen = flen;
	g_pendSeq = g_txSeq;
	g_pendTries = LM_TX_MAX_TRIES;
	g_pendTicks = 0;
	g_txSeq = (byte)((g_txSeq + 1) & (LM_SEQ_MOD - 1));
	LM_SendFrame(g_pendFrame, g_pendLen);
}

// Resend the outstanding frame verbatim (same seq) on NAK or ack-timeout.
static void LM_Retransmit(const char *why) {
	if (g_pendLen <= 0) return;
	if (g_pendTries-- <= 0) {
		g_pendLen = 0;
		g_txDropped++;
		ADDLOG_INFO(LOG_FEATURE_GENERAL,
			"LM ARQ gave up on seq %d (%s)", g_pendSeq, why);
		return;
	}
	g_pendTicks = 0;
	g_txResends++;
	LM_SendFrame(g_pendFrame, g_pendLen);
}

// Operator ACKed one of our data frames: '<' 'K' <seqNibble> '>'.
static void LM_OnAck(char seqChar) {
	int n = LM_HexNibble(seqChar);
	g_rxAcks++;
	if (g_pendLen > 0 && n == (int)g_pendSeq)
		g_pendLen = 0; // delivered + confirmed
}

// Operator NAKed: '<' 'N' <raw byte> '>' (its seq field is an internal value,
// not our nibble) -> retransmit the outstanding frame immediately.
static void LM_OnNak(void) {
	g_rxNaks++;
	LM_Retransmit("nak");
}

// Send a Motion_Actuate with the given selector. The ARQ layer delivers it
// reliably (retransmit on NAK/timeout until the operator ACKs).
static void LM_SendMotion(byte selector) {
	g_motionMsg[12] = (byte)(selector & 0x0f);
	LM_SendData(LM_DOOR_HDR, g_motionMsg, sizeof(g_motionMsg));
}

// Drive the door. open != 0 -> OPEN, else CLOSE.
static void LM_DoorCommand(int open) {
	g_doorTarget = open ? 1 : 0;
	LM_SendMotion(open ? LM_SEL_OPEN : LM_SEL_CLOSE);
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM door command -> %s (sel %d, seq %d)",
		open ? "OPEN" : "CLOSE", open ? LM_SEL_OPEN : LM_SEL_CLOSE, g_pendSeq);
}

// NOTE ON STOP: this operator accepts no stop over the host link. All 32
// Msg_S_Motion_Actuate variants (8 selectors x 2 flag bits) were tested both
// while idle and mid-travel, plus msg ids 0x284/0x285 and a direct door-state
// set -- the door always ran to its limit. The wall control's stop button
// reaches the operator over the separate accessory bus, not this link, and the
// myQ protocol itself exposes only open/close. So no stop command is offered.

// -------------------------------------------------------------- RX handler ---
// inner is the null-terminated text between '<' and '>' : TYPE SEQ hexbody
static void LM_HandleInner(char *inner, int innerLen) {
	byte body[LM_FRAME_MAX];
	int bodyLen, coreLen, plen;
	byte crcGot, crcCalc, *hdr, *payload;
	char typeChar;
	int seq;

	if (innerLen < 1) return;
	typeChar = inner[0];

	// ARQ control frames from the operator (short, no body): ACK / NAK of the
	// data frames WE sent. Handle before the data-frame length guard.
	if (typeChar == 'K') { if (innerLen >= 2) LM_OnAck(inner[1]); return; }
	if (typeChar == 'N') { LM_OnNak(); return; }

	if (innerLen < 2 + 2) return; // data frame: need TYPE SEQ + at least a byte
	seq = LM_HexNibble(inner[1]);

	bodyLen = LM_HexToBytes(inner + 2, body, sizeof(body));
	if (bodyLen < 6) return; // need hdr(4)+len(1)+crc(1) minimum

	coreLen = bodyLen - 1;           // everything but the trailing CRC
	crcGot = body[bodyLen - 1];
	crcCalc = LM_Crc8(body, coreLen);
	hdr = body;
	plen = body[4];
	payload = body + 5;

	g_rxFrames++;
	if (crcGot != crcCalc) {
		g_rxCrcErr++;
		ADDLOG_INFO(LOG_FEATURE_GENERAL,
			"LM RX BAD-CRC type=%c seq=%d hdr=%02X%02X%02X%02X len=%d got=%02X calc=%02X",
			typeChar, seq, hdr[0], hdr[1], hdr[2], hdr[3], plen, crcGot, crcCalc);
		return;
	}
	// coreLen must equal 5 (hdr+len) + plen; guard against truncation
	if (coreLen != 5 + plen) {
		ADDLOG_INFO(LOG_FEATURE_GENERAL,
			"LM RX len-mismatch hdr=%02X%02X%02X%02X declared=%d actual=%d",
			hdr[0], hdr[1], hdr[2], hdr[3], plen, coreLen - 5);
		return;
	}

	// ARQ: acknowledge received data frames ('P') so the operator stops
	// retransmitting them. Do not ACK acks/naks/keepalives.
	if (typeChar == 'P')
		LM_SendAck(inner[1]);

	ADDLOG_INFO(LOG_FEATURE_GENERAL,
		"LM RX type=%c seq=%d hdr=%02X%02X%02X%02X len=%d",
		typeChar, seq, hdr[0], hdr[1], hdr[2], hdr[3], plen);
	LM_NoteFrame(g_lastRxStr, (const byte *)inner, innerLen);
	LM_NoteFrame(g_ring[g_ringHead], (const byte *)inner, innerLen);
	g_ringHead = (byte)((g_ringHead + 1) % LM_RING_N);
	g_ringTotal++;

	// Decode door state -> channel.
	//
	// The operator reports position in a 0x0D TLV at the end of a len-16
	// msg 0x1C status frame. Two attribute ids carry it -- 0x0000 and 0x001A --
	// and BOTH must be decoded: 0x001A is emitted when the door settles at a
	// limit, but a door STOPPED PART-WAY is only reported via attr 0x0000. If you
	// decode 0x001A alone the driver happily reports "CLOSED" while the door is
	// actually sitting halfway open. (attr 0x0002 is a different, longer message
	// -- do not decode it here.)
	//
	// Layout: 01 |1C 00| <attr> | 01 06 <6-byte device id> | 0D 01 <state>
	//         ^subcmd ^msg id              ^TLV 0x0D value:
	//   0x00 = CLOSED   0x01 = OPEN   0x02 = MOVING   0x03 = STOPPED part-way
	if (plen == LM_STATUS_LEN && memcmp(hdr, LM_STATUS_HDR, 4) == 0
		&& payload[0] == 0x01 && payload[1] == 0x1c && payload[2] == 0x00
		&& (payload[3] == 0x1a || payload[3] == 0x00) && payload[4] == 0x00
		&& payload[13] == 0x0d && payload[14] == 0x01) {
		int st = payload[15];
		g_doorState = st;
		// keep the binary open/closed view only for the two settled states
		if (st == 0 || st == 1) {
			g_lastDir = st;
			if (g_doorTarget == g_lastDir)
				g_doorTarget = -1; // command confirmed by the operator
		}
		g_suppressChannelCb = 1; // this is feedback, not a user command
		CHANNEL_Set(g_statusChannel, st, 0);
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM door state = %s (%d, ch%d)",
			LM_StateName(st), st, g_statusChannel);
	}
}

void LiftMaster_RunFrame(void) {
	char inner[LM_FRAME_MAX];
	int avail, end, i, innerLen;

	avail = UART_GetDataSize();
	// Drop bytes until a frame start '<'.
	while (avail > 0 && UART_GetByte(0) != '<') {
		UART_ConsumeBytes(1);
		avail--;
	}
	if (avail < 2) return;

	// Look for the frame terminator '>'.
	end = -1;
	for (i = 1; i < avail; i++) {
		if (UART_GetByte(i) == '>') { end = i; break; }
	}
	if (end < 0) {
		// No complete frame yet. If it's grown implausibly long without a
		// terminator, drop the stale '<' so we can resync.
		if (avail > LM_FRAME_MAX)
			UART_ConsumeBytes(1);
		return;
	}

	innerLen = end - 1; // bytes strictly between '<' and '>'
	if (innerLen >= LM_FRAME_MAX) {
		UART_ConsumeBytes(end + 1); // oversized: discard the whole frame
		return;
	}
	for (i = 0; i < innerLen; i++)
		inner[i] = (char)UART_GetByte(1 + i);
	inner[innerLen] = 0;
	UART_ConsumeBytes(end + 1); // consume through '>'

	LM_HandleInner(inner, innerLen);
}

// -------------------------------------------------------------- commands -----
// LM_Send <hdrHex8> [payloadHex] : build a proper frame (CRC added) and send it.
// e.g.  LM_Send 01110211 00112233...   (header = 4 bytes = 8 hex chars)
static commandResult_t CMD_LM_Send(const void *context, const char *cmd,
		const char *args, int flags) {
	byte hdr[4], payload[LM_PAYLOAD_MAX];
	int plen = 0;
	const char *hdrStr, *plStr;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	hdrStr = Tokenizer_GetArg(0);
	if (LM_HexToBytes(hdrStr, hdr, 4) != 4) {
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM_Send: header must be 4 bytes (8 hex chars)");
		return CMD_RES_ERROR;
	}
	if (Tokenizer_GetArgsCount() >= 2) {
		plStr = Tokenizer_GetArg(1);
		plen = LM_HexToBytes(plStr, payload, sizeof(payload));
		if (plen < 0) {
			ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM_Send: bad payload hex");
			return CMD_RES_ERROR;
		}
	}
	// Go through the ARQ rather than emitting a bare frame with a free-running
	// seq. A raw send burns a sequence number even when the operator rejects the
	// frame, which desynchronises the link: the LPC then NAKs on sequence and
	// never evaluates the message at all. That made protocol sweeps unreliable --
	// roughly half the frames were NAK'd, so a "no response" result said nothing
	// about whether the message id/attribute was meaningful.
	LM_SendData(hdr, payload, plen);
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM TX (arq seq=%d) len=%d", g_pendSeq, plen);
	return CMD_RES_OK;
}

// LM_SendRaw <hex> : send literal bytes verbatim (lowest-level experimentation).
static commandResult_t CMD_LM_SendRaw(const void *context, const char *cmd,
		const char *args, int flags) {
	byte raw[LM_FRAME_MAX];
	int n, i;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	n = LM_HexToBytes(Tokenizer_GetArg(0), raw, sizeof(raw));
	if (n < 0) return CMD_RES_ERROR;
	for (i = 0; i < n; i++)
		UART_SendByte(raw[i]);
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM TX raw %d bytes", n);
	return CMD_RES_OK;
}

// LM_StatusChannel <ch> : choose which OBK channel receives the door state.
static commandResult_t CMD_LM_StatusChannel(const void *context, const char *cmd,
		const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	g_statusChannel = Tokenizer_GetArgInteger(0);
	LM_SetupDoorSwitchUI(); // move the web-UI switch to the new channel
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM status channel = %d", g_statusChannel);
	return CMD_RES_OK;
}

// LM_Door <0|1> : command the door closed(0)/open(1) with retry-until-confirmed.
static commandResult_t CMD_LM_Door(const void *context, const char *cmd,
		const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	LM_DoorCommand(Tokenizer_GetArgInteger(0) ? 1 : 0);
	return CMD_RES_OK;
}

// LM_Open / LM_Close: momentary door commands, one Motion_Actuate each.
// They are absolute-state, so commanding OPEN while already open is a no-op.
static commandResult_t CMD_LM_Open(const void *context, const char *cmd,
		const char *args, int flags) {
	LM_DoorCommand(1);
	return CMD_RES_OK;
}
static commandResult_t CMD_LM_Close(const void *context, const char *cmd,
		const char *args, int flags) {
	LM_DoorCommand(0);
	return CMD_RES_OK;
}
// LM_Stat : log driver + ARQ counters (RX/TX frames, acks/naks, pending, state).
// Lets us verify link health over HTTP without SWD.
static commandResult_t CMD_LM_Stat(const void *context, const char *cmd,
		const char *args, int flags) {
	ADDLOG_INFO(LOG_FEATURE_GENERAL,
		"LM rx=%u crcErr=%u tx=%u ack=%u rxAck=%u rxNak=%u resend=%u drop=%u "
		"pend=%d pendSeq=%d txSeq=%d lastDir=%d target=%d",
		g_rxFrames, g_rxCrcErr, g_txFrames, g_txAcks, g_rxAcks, g_rxNaks,
		g_txResends, g_txDropped, g_pendLen, g_pendSeq, g_txSeq,
		g_lastDir, g_doorTarget);
	return CMD_RES_OK;
}

// Channel callback: an external write to the door channel is a COMMAND.
// This is the Home Assistant path -- an MQTT cover publishes one payload to one
// command topic, which maps onto OpenBeken's "<clientId>/<channel>/set":
//     publish 1 -> open,  publish 0 -> close
// Anything else (2/3 = the moving / stopped-part-way values we publish as state)
// is ignored, so echoing our own state back at us can never actuate the door.
//
// Writes we make ourselves from the RX status decode are feedback, not commands,
// and are skipped via g_suppressChannelCb.
void LiftMaster_OnChannelChanged(int channel, int value) {
	if (channel != g_statusChannel) return;
	if (g_suppressChannelCb) { g_suppressChannelCb = 0; return; }
	if (value != 0 && value != 1) return;   // only 0/1 are commands
	if (value == g_lastDir) return;         // already in that state; operator would no-op
	LM_DoorCommand(value);
}

// ----------------------------------------------------------- lifecycle -------
// The door-state channel is READ-ONLY on purpose: a garage door is not an on/off
// switch, and a toggle personality misrepresents it (there is no single "off"
// action -- OPEN/CLOSE/STOP are three distinct momentary commands, mirrored by
// the buttons on the index page). The channel exists so the real door position
// is published: OBK reports channel values over MQTT, so Home Assistant can use
// it as the state source (0=closed, 1=open) while driving the door by publishing
// LM_Open / LM_Close to the device's command topic.
//
// Status frames are event-driven from the LPC and arrive no matter who moved the
// door (our command, the wall control, or an RF remote), so the channel always
// tracks reality.
static void LM_SetupDoorSwitchUI(void) {
	CHANNEL_SetType(g_statusChannel, ChType_ReadOnly);
	CHANNEL_SetLabel(g_statusChannel, "Door", 1);
}

void LiftMaster_Init(void) {
	LM_BuildCrcTable();
	UART_InitUART(LM_BAUD, LM_PARITY, false);
	UART_InitReceiveRingBuffer(LM_RX_RING);
	LM_SetupDoorSwitchUI();

	// Door state is EVENT-DRIVEN: the operator reports only on change, so after a
	// reboot we genuinely do not know where the door is until the first status
	// frame arrives. Publish an explicit unknown sentinel rather than leaving the
	// channel at its default 0 -- otherwise Home Assistant would confidently show
	// "closed" while the door is actually open.
	g_suppressChannelCb = 1;
	CHANNEL_Set(g_statusChannel, LM_STATE_UNKNOWN, 0);

	//cmddetail:{"name":"LM_Send","args":"[hdrHex8][payloadHex]",
	//cmddetail:"descr":"Build a msg1210/Saturn frame (CRC-8 added) and send it to the door board.",
	//cmddetail:"fn":"CMD_LM_Send","file":"driver/drv_liftmaster.c","requires":""}
	CMD_RegisterCommand("LM_Send", CMD_LM_Send, NULL);
	//cmddetail:{"name":"LM_SendRaw","args":"[hex]",
	//cmddetail:"descr":"Send literal bytes on the host UART (raw experimentation).",
	//cmddetail:"fn":"CMD_LM_SendRaw","file":"driver/drv_liftmaster.c","requires":""}
	CMD_RegisterCommand("LM_SendRaw", CMD_LM_SendRaw, NULL);
	//cmddetail:{"name":"LM_StatusChannel","args":"[channel]",
	//cmddetail:"descr":"Set the OBK channel that receives the decoded door state (0=closed,1=open).",
	//cmddetail:"fn":"CMD_LM_StatusChannel","file":"driver/drv_liftmaster.c","requires":""}
	CMD_RegisterCommand("LM_StatusChannel", CMD_LM_StatusChannel, NULL);
	//cmddetail:{"name":"LM_Door","args":"[0|1]",
	//cmddetail:"descr":"Command the door closed(0)/open(1); retransmits until the operator confirms.",
	//cmddetail:"fn":"CMD_LM_Door","file":"driver/drv_liftmaster.c","requires":""}
	CMD_RegisterCommand("LM_Door", CMD_LM_Door, NULL);
	//cmddetail:{"name":"LM_Stat","args":"",
	//cmddetail:"descr":"Log LiftMaster driver + ARQ counters (rx/tx/ack/nak/pending/state).",
	//cmddetail:"fn":"CMD_LM_Stat","file":"driver/drv_liftmaster.c","requires":""}
	CMD_RegisterCommand("LM_Stat", CMD_LM_Stat, NULL);
	//cmddetail:{"name":"LM_Open","args":"",
	//cmddetail:"descr":"Open the door (momentary Motion_Actuate, selector 1).",
	//cmddetail:"fn":"CMD_LM_Open","file":"driver/drv_liftmaster.c","requires":""}
	CMD_RegisterCommand("LM_Open", CMD_LM_Open, NULL);
	//cmddetail:{"name":"LM_Close","args":"",
	//cmddetail:"descr":"Close the door (momentary Motion_Actuate, selector 0).",
	//cmddetail:"fn":"CMD_LM_Close","file":"driver/drv_liftmaster.c","requires":""}
	CMD_RegisterCommand("LM_Close", CMD_LM_Close, NULL);

	ADDLOG_INFO(LOG_FEATURE_GENERAL,
		"LiftMaster (Saturn/msg1210) driver started @ %d 8N1, status->ch%d",
		LM_BAUD, g_statusChannel);
}

void LiftMaster_RunEverySecond(void) {
	// ARQ ack-timeout backstop: NAKs trigger an immediate resend in the RX path;
	// this catches a silently-dropped frame (no ACK, no NAK) by retransmitting
	// the outstanding frame until it is ACKed or the retry budget is spent.
	if (g_pendLen > 0 && ++g_pendTicks >= LM_TX_TMO_TICKS)
		LM_Retransmit("timeout");

	if ((g_rxFrames | g_txFrames) && (g_rxFrames % 20 == 0)) {
		ADDLOG_DEBUG(LOG_FEATURE_GENERAL,
			"LM stats rx=%u crcErr=%u tx=%u ack=%u rxAck=%u rxNak=%u resend=%u drop=%u",
			g_rxFrames, g_rxCrcErr, g_txFrames, g_txAcks,
			g_rxAcks, g_rxNaks, g_txResends, g_txDropped);
	}
}

// Index page: two momentary door buttons and the door state, nothing else.
//
// The protocol diagnostics (ARQ counters, last frames, RX ring) are the only
// window onto the link once the board is installed in an operator -- there is no
// serial console -- so they are kept, but behind "?lmdebug=1" instead of dumped
// on the main page.
void LiftMaster_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	char tmp[8];

	// bPreState runs before the page body: handle a button press here so the
	// state shown below already reflects the action.
	if (bPreState) {
		if (http_getArg(request->url, "lmact", tmp, sizeof(tmp))) {
			if (!strcmp(tmp, "open"))       LM_DoorCommand(1);
			else if (!strcmp(tmp, "close")) LM_DoorCommand(0);
		}
		return;
	}

	// Two momentary buttons. A door is not an on/off switch: OPEN and CLOSE are
	// distinct absolute-state commands, and the operator ignores the one matching
	// the state it is already in. (No STOP button -- the operator does not accept
	// a stop over this link; see docs/obk_command_path.md.)
	hprintf255(request, "<h4>Door is %s</h4>", LM_StateName(g_doorState));
	poststr(request, "<table><tr>");
	poststr(request, "<td><form action=\"index\">"
		"<input type=\"hidden\" name=\"lmact\" value=\"open\">"
		"<input style=\"background-color:#6f7780;\" type=\"submit\" value=\"OPEN\"/>"
		"</form></td>");
	poststr(request, "<td><form action=\"index\">"
		"<input type=\"hidden\" name=\"lmact\" value=\"close\">"
		"<input style=\"background-color:#6f7780;\" type=\"submit\" value=\"CLOSE\"/>"
		"</form></td>");
	poststr(request, "</tr></table>");

	if (!http_getArg(request->url, "lmdebug", tmp, sizeof(tmp)))
		return;

	// ---- diagnostics (only with ?lmdebug=1) ----
	hprintf255(request, "<h5>state=%d lastDir=%d target=%d ch%d</h5>",
		g_doorState, g_lastDir, g_doorTarget, g_statusChannel);
	hprintf255(request, "<h5>rx=%u crcErr=%u tx=%u ack=%u rxAck=%u rxNak=%u</h5>",
		g_rxFrames, g_rxCrcErr, g_txFrames, g_txAcks, g_rxAcks, g_rxNaks);
	hprintf255(request, "<h5>resend=%u drop=%u pend=%d pendSeq=%d txSeq=%d</h5>",
		g_txResends, g_txDropped, g_pendLen, g_pendSeq, g_txSeq);
	hprintf255(request, "<h5>lastRX &lt;%s&gt;</h5>", g_lastRxStr);
	hprintf255(request, "<h5>lastTX &lt;%s&gt;</h5>", g_lastTxStr);

	// Recent RX frames, oldest first, so a single poll recovers a door cycle.
	hprintf255(request, "<h5>ring total=%u</h5>", g_ringTotal);
	{
		uint32_t have = (g_ringTotal < LM_RING_N) ? g_ringTotal : LM_RING_N;
		uint32_t i;
		for (i = 0; i < have; i++) {
			// walk back 'have' slots from the write head
			byte slot = (byte)((g_ringHead + LM_RING_N - have + i) % LM_RING_N);
			hprintf255(request, "<h5>R%02u &lt;%s&gt;</h5>",
				(unsigned)(g_ringTotal - have + i), g_ring[slot]);
		}
	}
}

#if ENABLE_HA_DISCOVERY
// Publish a Home Assistant COVER (device_class: garage) for the door.
//
// Without this the generic channel discovery only offers a read-only sensor,
// which cannot open or close anything. OpenBeken's garage helper defaults to the
// string payloads OPEN/CLOSE/STOP and states "open"/"closed"; our channel is
// numeric (see LiftMaster_OnChannelChanged), so those fields are overridden:
//   command: 1 = open, 0 = close      state: 1 = open, 0 = closed
// payload_stop is REMOVED -- this operator does not accept a stop over the host
// link (see pwnLiftMaster docs/obk_command_path.md), and a stop button that
// silently does nothing is worse than no button.
void LiftMaster_OnHassDiscovery(const char *topic) {
	char stateTopic[96], cmdTopic[96];
	HassDeviceInfo *info;
	const char *clientId = CFG_GetMQTTClientId();

	// State comes from the channel; COMMANDS go to the "cmnd/" topic, NOT to
	// "<channel>/set". Channel writes are edge-triggered -- OpenBeken only fires
	// onChannelChanged when the value actually changes -- so publishing "close"
	// while the channel already read 0 silently did nothing. (That is exactly the
	// bug where OPEN worked and CLOSE did not.) A door command must fire every
	// time it is sent, so we use "cmnd/<clientId>/LM_Door" with payload 1/0,
	// which executes on every publish regardless of current state.
	snprintf(stateTopic, sizeof(stateTopic), "%s/%d/get", clientId, g_statusChannel);
	snprintf(cmdTopic, sizeof(cmdTopic), "cmnd/%s/LM_Door", clientId);

	info = hass_createGarageEntity(stateTopic, cmdTopic, "Door");
	if (info == 0)
		return;

	cJSON_ReplaceItemInObject(info->root, "payload_open", cJSON_CreateString("1"));
	cJSON_ReplaceItemInObject(info->root, "payload_close", cJSON_CreateString("0"));
	cJSON_ReplaceItemInObject(info->root, "state_open", cJSON_CreateString("1"));
	cJSON_ReplaceItemInObject(info->root, "state_closed", cJSON_CreateString("0"));
	// Home Assistant DEFAULTS payload_stop to "STOP" -- removing the key is not
	// enough, it must be explicit null, or HA renders a stop button that would do
	// nothing (this operator accepts no stop over the host link).
	cJSON_ReplaceItemInObject(info->root, "payload_stop", cJSON_CreateNull());

	MQTT_QueuePublish(topic, info->channel, hass_build_discovery_json(info),
		OBK_PUBLISH_FLAG_RETAIN);
	hass_free_device_info(info);

	ADDLOG_INFO(LOG_FEATURE_GENERAL, "LM published HA cover discovery (%s / %s)",
		stateTopic, cmdTopic);
}
#endif

void LiftMaster_Shutdown(void) {
	// UART is shared infrastructure; nothing to free here.
}
