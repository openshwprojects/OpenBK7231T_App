
#include "../obk_config.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "../new_common.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"
#include "../new_cfg.h"
#include "../new_pins.h"

static int g_testCommand = -1;
static int g_testStage = 0;
static int g_testInterval = 100;
static int g_curTime = 0;
static int g_test_ok = 0;
static int g_test_errors = 0;

const char *g_testCommands[] = {
	"testMallocFree",
	"testStrdup",
	"testRealloc",
	"json_test",
	"lfs_test1",
	"TestSprintfForInteger",
	"TestSprintfForHex",
	"TestSprintfForFloat",
	"TestStrcpy",
	"TestStrncpy",
	"TestStrcmp",
	"TestStrlen",
	"TestStrcat",
	"TestMemset",
	"TestMemcpy",
	"TestMemcmp",
	"TestChannelSetToExpression",
	"TestParseIP",
	"TestIPtoStr"
}; 
static int g_numTestCommands = sizeof(g_testCommands) / sizeof(g_testCommands[0]);

static commandResult_t Test_Cmd_Start(const void * context, const char *cmd, const char *args, int cmdFlags) {
	g_testCommand = 0;
	g_testStage = 0;
	g_curTime = 0;
	g_test_ok = 0;
	g_test_errors = 0;
	if (args && *args) {
		g_testInterval = atoi(args);
	}
	return CMD_RES_OK;
}

void Test_AppendInformationToHTTPIndexPage(http_request_t *request)
{

	if (http_getArgInteger(request->url, "restart")) {
		Test_Cmd_Start(0, 0, 0, 0);
	}
	poststr(request, "<hr><table style='width:100%'>");

	int progress = (g_testCommand >= 0) ? (g_testCommand * 100) / g_numTestCommands : 100;
	const char *color = (g_test_errors == 0) ? "green" : "red";

	poststr(request, "<tr><td><b>Test Progress</b></td><td style='text-align: right;'>");
	hprintf255(request, "<span style='color:%s;'>%d%%</span></td></tr>", color, progress);

	poststr(request, "<tr><td><b>Tests Passed</b></td><td style='text-align: right;'>");
	hprintf255(request, "<span style='color:green;'>%d</span></td></tr>", g_test_ok);

	poststr(request, "<tr><td><b>Tests Failed</b></td><td style='text-align: right;'>");
	hprintf255(request, "<span style='color:red;'>%d</span></td></tr>", g_test_errors);

	poststr(request, "</table><hr>");

	poststr(request, "<form method='get'><input type='hidden' name='restart' value='1'>");
	poststr(request, "<input type='submit' value='Restart Self Test'></form>");
}


// backlog startDriver Test; StartTest 100;
void Test_Init(void) {
	CMD_RegisterCommand("StartTest", Test_Cmd_Start, NULL);

}
void Test_RunQuickTick(void) {
	g_curTime += g_deltaTimeMS;
	if (g_curTime < g_testInterval)
		return;
	g_curTime = 0;
	if (g_testCommand != -1) {
		const char *cmd = g_testCommands[g_testCommand];
		if (g_testStage == 0) {
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "[%i/%i] Going to test %s!", g_testCommand, g_numTestCommands, cmd);
			g_testStage = 1;
		}
		else if(g_testStage == 1) {
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "[%i/%i] Executing %s...", g_testCommand, g_numTestCommands, cmd);
			commandResult_t res = CMD_ExecuteCommand(cmd, 0);
			if (res == CMD_RES_OK)
			{
				addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "[%i/%i] Result OK!", g_testCommand, g_numTestCommands);
				g_test_ok++;
			}
			else {
				addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "[%i/%i] Result Error %i!", g_testCommand, g_numTestCommands, res);
				g_test_errors++;
			}
			g_testStage = 2;
		}
		else {
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "[%i/%i] Finished test of %s!", g_testCommand, g_numTestCommands, cmd);
			g_testStage = 0;
			g_testCommand++;
			if (g_testCommand >= g_numTestCommands) {
				g_testCommand = -1;
			}
		}
	}
}



