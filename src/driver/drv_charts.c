#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "drv_ntp.h"
/*
// Sample 1
// single variable chart
startDriver charts
// chart with max 16 samples, 1 variable and single axis
chart_create 16 1 1
// set the temperature variable with axis
chart_setVar 0 "Temperature" "axtemp"
// setup axis
// axis_index, name, flags, label
chart_setAxis 0 "axtemp" 0 "Temperature (C)"
// for demonstration purposes, add some data at fixed times
// First argument is NTP time value
chart_add 1725606094 20
chart_add 1725616094 22
chart_add 1725626094 26
chart_add 1725636094 30
chart_add 1725646094 28
chart_add 1725656094 27

// Sample 2
// Three temperature variables chart
startDriver charts
// chart with max 16 samples, 3 variables and single axis
chart_create 16 3 1
// set variables along with their axis
chart_setVar 0 "Kitchen" "axtemp"
chart_setVar 1 "Outside" "axtemp"
chart_setVar 2 "Bedroom" "axtemp"
// setup axis
// axis_index, name, flags, label
chart_setAxis 0 "axtemp" 0 "Temperature (C)"
// for demonstration purposes, add some data at fixed times
// First argument is NTP time value
chart_add 1725606094 20 15 22
chart_add 1725616094 22 16 23
chart_add 1725626094 26 17 24
chart_add 1725636094 30 14 25
chart_add 1725646094 28 13 22
chart_add 1725656094 27 15 21

// Sample 3
// Two temperatures and one humidity with separate Temperature/Humidity axes

startDriver charts
// chart with max 16 samples, 3 variables and two separate vertical axes
chart_create 16 3 2
// set variables along with their axes
chart_setVar 0 "Room T" "axtemp"
chart_setVar 1 "Outside T" "axtemp"
chart_setVar 2 "Humidity" "axhum"
// setup axes
// axis_index, name, flags, label
chart_setAxis 0 "axtemp" 0 "Temperature (C)"
// flags 1 means this axis is on the right
chart_setAxis 1 "axhum" 1 "Humidity (%)"
// for demonstration purposes, add some data at fixed times
// First argument is NTP time value
chart_add 1725606094 20 15 89
chart_add 1725616094 22 16 88
chart_add 1725626094 26 17 91
chart_add 1725636094 30 14 92
chart_add 1725646094 28 13 92
chart_add 1725656094 27 15 91


// Sample 4
// Battery voltage + pressure
startDriver charts
// chart with max 16 samples, 2 variables and 2 axes
chart_create 16 2 2
// set variables along with their axis
chart_setVar 0 "Battery Voltage" "axvolt"
chart_setVar 1 "Pressure" "axpress"
// setup axis
// axis_index, name, flags, label
chart_setAxis 0 "axvolt" 0 "Battery Voltage (V)"
chart_setAxis 1 "axpress" 1 "Pressure (hPa)"
// for demonstration purposes, add some data at fixed times
// First argument is NTP time value
chart_add 1725606094 3.8 1013
chart_add 1725616094 3.7 1011
chart_add 1725626094 3.7 1012
chart_add 1725636094 3.6 1015
chart_add 1725646094 3.5 1013
chart_add 1725656094 3.4 1014

// Sample 5 - try 3 axes
startDriver charts
// chart with max 16 samples, 3 variables and 3 separate vertical axes
chart_create 16 3 3
// set variables along with their axes
chart_setVar 0 "Voltage" "axvolt"
chart_setVar 1 "Current" "axcurr"
chart_setVar 2 "Power" "axpower"
// setup axes
// axis_index, name, flags, label
chart_setAxis 0 "axvolt" 0 "Voltage (V)"
chart_setAxis 1 "axcurr" 1 "Current (A)"
chart_setAxis 2 "axpower" 2 "Power (W)"
// for demonstration purposes, add some data at fixed times
// First argument is NTP time value
chart_add 1725606094 12 0.5 6
chart_add 1725616094 11.8 0.52 6.14
chart_add 1725626094 11.5 0.54 6.21
chart_add 1725636094 11.3 0.55 6.22
chart_add 1725646094 11.1 0.56 6.22
chart_add 1725656094 10.9 0.58 6.32

*/
/*
// Sample 6
// This demonstrated how to save channel values to charts.
// No more fixed debug data!
startDriver charts
startDriver NTP
waitFor NTPState 1
chart_create 16 1 1
chart_setVar 0 "Temperature" "axtemp"
chart_setAxis 0 "axtemp" 0 "Temperature (C)"

// in a loop
again:
// This assumes that $CH1 is temperature_div10
// chart_addNow will take time from the NTP driver
chart_addNow $CH1*0.1
// every 10 seconds
delay_s 10
goto again


*/
/*
// Sample 7
// Just like previous sample, but uses repeating event instead of loop
startDriver charts
startDriver NTP
waitFor NTPState 1
chart_create 16 1 1
chart_setVar 0 "Temperature" "axtemp"
chart_setAxis 0 "axtemp" 0 "Temperature (C)"

// every 10 seconds, -1 means infinite repeats
addRepeatingEvent 10 -1 chart_addNow $CH1*0.1



*/
/*
// Sample 8
// DHT11 setup
IndexRefreshInterval 100000
startDriver charts
startDriver NTP
waitFor NTPState 1
chart_create 48 2 2
// set variables along with their axes
chart_setVar 0 "Temperature" "axtemp"
chart_setVar 1 "Humidity" "axhum"
// setup axes
// axis_index, name, flags, label
chart_setAxis 0 "axtemp" 0 "Temperature (C)"
// flags 1 means this axis is on the right
chart_setAxis 1 "axhum" 1 "Humidity (%)"

// every 60 seconds, -1 means infinite repeats
// assumes that $CH1 is temperature div10 and $CH2 is humidity
addRepeatingEvent 60 -1 chart_addNow $CH1*0.1 $CH2



*/
/*
// Sample 9
// Random numbers

IndexRefreshInterval 100000

startDriver charts
startDriver NTP
//waitFor NTPState 1
chart_create 16 1 1
chart_setVar 0 "Number" "ax"
chart_setAxis 0 "ax" 0 "Number"


again:
setChannel 5 $rand*0.001
chart_addNow $CH5
delay_s 1
goto again



*/
#define AX_RIGHT 1
typedef struct var_s {
	char *title;
	char *axis;
	float *samples;
} var_t;

typedef struct axis_s {
	char *name;
	char *label;
	int flags;
} axis_t;

typedef struct chart_s {
	int maxSamples;
	int nextSample;
	int lastSample;
	time_t *times;
	int numVars;
	var_t *vars;
	int numAxes;
	axis_t *axes;
} chart_t;

chart_t *g_chart = 0;

void Chart_Free(chart_t **ptr) {
	chart_t *s = *ptr;
	if (!s) {
		return; 
	}
	if (s->axes) {
		for (int i = 0; i < s->numAxes; i++) {
			if (s->axes[i].label) {
				free(s->axes[i].label);
			}
			if (s->axes[i].name) {
				free(s->axes[i].name);
			}
		}
		free(s->axes);
	}
	if (s->vars) {
		for (int i = 0; i < s->numVars; i++) {
			if (s->vars[i].title) {
				free(s->vars[i].title);
			}
			if (s->vars[i].samples) {
				free(s->vars[i].samples);
			}
		}
		free(s->vars);
	}
	if (s->times) {
		free(s->times);
	}
	free(s);
	*ptr = 0;
}
byte *ZeroMalloc(unsigned int size) {
	byte *r = (byte*)malloc(size);
	if (r == 0)
		return 0;
	memset(r, 0, size);
	return r;
}
chart_t *Chart_Create(int maxSamples, int numVars, int numAxes) {
	chart_t *s = (chart_t *)ZeroMalloc(sizeof(chart_t));
	if (!s) {
		return NULL;
	}
	s->vars = (var_t *)ZeroMalloc(sizeof(var_t) * numVars);
	if (!s->vars) {
		free(s);
		return NULL; 
	}
	s->axes = (axis_t *)ZeroMalloc(sizeof(axis_t) * numAxes);
	if (!s->axes) {
		free(s->vars);
		free(s);
		return NULL;
	}
	s->times = (time_t *)ZeroMalloc(sizeof(time_t) * maxSamples);
	if (!s->times) {
		free(s->axes);
		free(s->vars);
		free(s);
		return NULL; 
	}

	for (int i = 0; i < numVars; i++) {
		s->vars[i].samples = (float*)ZeroMalloc(sizeof(float) * maxSamples);
		if (s->vars[i].samples == 0) {
			for (int j = 0; j < i; j++) {
				free(s->vars[j].samples);
			}
			free(s->times);
			free(s->axes);
			free(s->vars);
			free(s);
			return NULL;
		}
	}
	s->numAxes = numAxes;
	s->numVars = numVars;
	s->maxSamples = maxSamples;
	s->nextSample = 0;
	s->lastSample = 0; 
	return s;
}
void Chart_SetAxis(chart_t *s, int idx, const char *name, int flags, const char *label) {
	if (!s || idx >= s->numAxes) {
		return;
	}
	s->axes[idx].name = strdup(name);
	s->axes[idx].label = strdup(label);
	s->axes[idx].flags = flags;
}
void Chart_SetVar(chart_t *s, int idx, const char *title, const char *axis) {
	if (!s || idx >= s->numVars) {
		return;
	}
	s->vars[idx].title = strdup(title);
	s->vars[idx].axis = strdup(axis);
}
void Chart_SetSample(chart_t *s, int idx, float value) {
	if (!s || idx >= s->numVars) {
	bk_printf("DEBUG CHARTS: ERROR - Chart_SetSample ixd=%i /  s->numVars=%i / value=%f\n",idx, s->numVars,value); 
		return;
	}
	bk_printf("DEBUG CHARTS: OK - Chart_SetSample ixd=%i /  s->numVars=%i  / value=%f\n",idx, s->numVars,value); 
	s->vars[idx].samples[s->nextSample] = value;
}
void Chart_AddTime(chart_t *s, time_t time) {
	if (!s) {
		return;
	}
	s->times[s->nextSample] = time;
	s->nextSample = (s->nextSample + 1) % s->maxSamples;
	if (s->lastSample == s->nextSample) {
		s->lastSample = (s->lastSample + 1) % s->maxSamples;
	}
}
void Chart_Iterate(chart_t *s, int index, void (*callback)(float *val, time_t *time, void *userData), void *userData) {
	if (!s) {
		return;
	}
	int start = s->lastSample;
	int end = s->nextSample;

	if (start <= end) {
		for (int i = start; i < end; i++) {
			callback(&s->vars[index].samples[i], &s->times[i], userData);
		}
	} else {
		for (int i = start; i < s->maxSamples; i++) {
			callback(&s->vars[index].samples[i], &s->times[i], userData);
		}
		for (int i = 0; i < end; i++) {
			callback(&s->vars[index].samples[i], &s->times[i], userData);
		}
	}
}
void Chart_DisplayLabel(float *val, time_t *time, void *userData) {
	char buffer[64];
	http_request_t *request = (http_request_t *)userData;
	if (request->userCounter != 0) {
		poststr(request, ",");
	}
	request->userCounter++;
	snprintf(buffer, sizeof(buffer), "%ld", (long)(*time));					// don't transmit too much data, use only the timestamps here and handle conversion later  ....
	poststr(request, buffer);
}
void Chart_DisplayData(float *val, time_t *time, void *userData) {
	char buffer[32];
	http_request_t *request = (http_request_t *)userData;
	if (request->userCounter != 0) {
		poststr(request, ",");
	}
	request->userCounter++;
	snprintf(buffer, sizeof(buffer), "%.2f", *val);
	poststr(request, buffer);
}
void Chart_Display(http_request_t *request, chart_t *s) {
	char buffer[64];

	if (s == 0) {
		poststr(request, "<h4>Chart is NULL</h4>");
		return;
	}
	for (int i = 0; i < s->numAxes; i++) {
		if (s->axes[i].label == 0) {
			poststr(request, "<h4>No axes set</h4>");
			return;
		}
	}
	for (int i = 0; i < s->numVars; i++) {
		if (s->vars[i].title == 0) {
			poststr(request, "<h4>No vars set</h4>");
			return;
		}
	}

/*
	// on every "state" request, JS code will be loaded and canvas is redrawn
	// this leads to a flickering graph
	// so put this right below the "state" div
	// with a "#ifdef 
	// drawback : We need to take care, if driver is loaded and canvas will be displayed only on a reload of the page
	// or we might try and hide/unhide it ...
	poststr(request, "<canvas id=\"obkChart\" width=\"400\" height=\"200\"></canvas>");
	poststr(request, "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>");
*/
	poststr(request, "<input type='hidden' id='chartlabels' value='");
	request->userCounter = 0;
	Chart_Iterate(s, 0, Chart_DisplayLabel, request);
	poststr(request, "'>");
	for (int i = 0; i < s->numVars; i++) {
		hprintf255(request, "<input type='hidden' id='chartdata%i' value='",i);
		request->userCounter = 0;
		Chart_Iterate(s, i,  Chart_DisplayData, request);
		poststr(request, "'>");
	}
	poststr(request, "<script>");
	poststr(request, "function cha() {");
	poststr(request, "var labels =document.getElementById('chartlabels').value.split(/\s*,\s*/).map(Number).map((x)=>new Date(x * 1000).toLocaleTimeString());"); // we transmitted only timestamps, let Javascript do the work to convert them ;-)
	for (int i = 0; i < s->numVars; i++) {
		hprintf255(request, "var data%i = document.getElementById('chartdata%i').value.split(/\s*,\s*/).map(Number);",i,i);	
	}
	poststr(request, "if (! window.obkChartInstance) {");
	poststr(request, "console.log('Initializing chart');");
	poststr(request, "var ctx = document.getElementById('obkChart');");
	poststr(request, "if (ctx.style.display=='none') ctx.style.display='block';");
	poststr(request, "ctx =ctx.getContext('2d');");
	poststr(request, "window.obkChartInstance = new Chart(ctx, {");
	poststr(request, "    type: 'line',");
	poststr(request, "    data: {");
	poststr(request, "        labels: labels,");
	poststr(request, "        datasets: [");
	for (int i = 0; i < s->numVars; i++) {
		if (i) {
			poststr(request, ",");
		}
		poststr(request, "{");
		hprintf255(request, "            label: '%s',", s->vars[i].title);
		hprintf255(request, "            data: data%i,",i);
		if (i == 2) {
			poststr(request, "                borderColor: 'rgba(155, 33, 55, 1)',");
		}
		else if (i == 1) {
			poststr(request, "                borderColor: 'rgba(232, 122, 232, 1)',");
		}
		else {
			poststr(request, "            borderColor: 'rgba(75, 192, 192, 1)',");
		}
		hprintf255(request, "                yAxisID: '%s',", s->vars[i].axis);
		poststr(request, "            fill: false");
		poststr(request, "        }");
	}
	poststr(request, "]");
	poststr(request, "    },");
	poststr(request, "    options: {");
	poststr(request, "        animation: false,");		// for me it's annoying, if on every refresh the graph is "animated"
	poststr(request, "        scales: {");
	poststr(request, "            x: {");
	poststr(request, "                type: 'category',");  
	poststr(request, "            }");
	for (int i = 0; i < s->numAxes; i++) {
		poststr(request, ",");
		hprintf255(request, "            %s: {",  s->axes[i].name);
		if (s->axes[i].flags & AX_RIGHT) {
			poststr(request, "                position: 'right',");
		} else {
			poststr(request, "                position: 'left',");
		}
		if (s->axes[i].label) {
			poststr(request, "                title: {");
			poststr(request, "                    display: true,");
			hprintf255(request, "                    text: '%s'", s->axes[i].label);
			poststr(request, "                },");
		}
		if (i != 0) {
			poststr(request, "                grid: {");
			poststr(request, "                    drawOnChartArea: false"); // Avoid grid lines overlapping
			poststr(request, "                },");
		}
		poststr(request, "                beginAtZero: false");
		poststr(request, "            }");
	}
	poststr(request, "        }");
	poststr(request, "    }");
	poststr(request, "});\n");
	poststr(request, "Chart.defaults.color = '#099'; ");  // Issue #1375, add a default color to improve readability (applies to: dataset names, axis ticks, color for axes title, (use color: '#099')
	poststr(request, "}\n");
	poststr(request, "else {\n");
	poststr(request, "console.log('Updating chart');\n");
	poststr(request, "	window.obkChartInstance.data.labels=labels;\n");
	for (int i = 0; i < s->numVars; i++) {
		hprintf255(request, "	window.obkChartInstance.data.datasets[%i].data=data%i;\n",i,i);	
	}
	poststr(request, "	window.obkChartInstance.update();\n");
	poststr(request, "}\n}");
	poststr(request, "</script>");
	poststr(request, "<style onload='cha();'></style>");

}
void DRV_Charts_AddToHtmlPage_Test(http_request_t *request) {

	// chart_create [NumSamples] [NumVariables] [NumAxes]
	// chart_create 16 3 2
	chart_t *s = Chart_Create(16, 3, 2);
	// chart_setVar [VarIndex] [DisplayName] [AxisCode]
	// chart_setVar 0 "Room t" "axtemp"
	// chart_setVar 1 "Outside T" "axtemp"
	// chart_setVar 2 "Humidity" "axhum"
	Chart_SetVar(s, 0, "Room T", "axtemp");
	Chart_SetVar(s, 1, "Outside T", "axtemp");
	Chart_SetVar(s, 2, "Humidity", "axhum");
	// chart_setAxis [AxisIndex] [AxisCode] [Flags] [AxisLabel]
	// chart_setAxis 0 "axtemp" 0 "Temperature ('C)"
	// chart_setAxis 1 "axtemp" 1 "Humidity (%s)"
	Chart_SetAxis(s, 0, "axtemp", 0, "Temperature (C)");
	Chart_SetAxis(s, 1, "axhum", AX_RIGHT, "Humidity (%)");

	// chart_addNow [Var0] [Var1] [Var2] [VarN]
	// chart_add [Time] [Var0] [Var1] [Var2] [VarN]
	Chart_SetSample(s, 0, 20);
	Chart_SetSample(s, 1, 15);
	Chart_SetSample(s, 2, 89);
	Chart_AddTime(s, 1725606094);
	Chart_SetSample(s, 0, 22);
	Chart_SetSample(s, 1, 16);
	Chart_SetSample(s, 2, 88);
	Chart_AddTime(s, 1725616094);
	Chart_SetSample(s, 0, 26);
	Chart_SetSample(s, 1, 17);
	Chart_SetSample(s, 2, 91);
	Chart_AddTime(s, 1725626094);
	Chart_SetSample(s, 0, 30);
	Chart_SetSample(s, 1, 14);
	Chart_SetSample(s, 2, 92);
	Chart_AddTime(s, 1725636094);
	Chart_SetSample(s, 0, 28);
	Chart_SetSample(s, 1, 13);
	Chart_SetSample(s, 2, 92);
	Chart_AddTime(s, 1725646094);
	Chart_SetSample(s, 0, 27);
	Chart_SetSample(s, 1, 15);
	Chart_SetSample(s, 2, 91);
	Chart_AddTime(s, 1725656094);
	Chart_Display(request, s);
	Chart_Free(&s);
}
// startDriver Charts
void DRV_Charts_AddToHtmlPage(http_request_t *request) {
	if (0) {
		DRV_Charts_AddToHtmlPage_Test(request);
		return;
	}
	if (g_chart) {
		Chart_Display(request, g_chart);
	}
}

static commandResult_t CMD_Chart_Create(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if(Tokenizer_GetArgsCount()<=1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int numSamples = Tokenizer_GetArgInteger(0);
	int numVars = Tokenizer_GetArgInteger(1);
	int numAxes = Tokenizer_GetArgInteger(2);

	Chart_Free(&g_chart);
	g_chart = Chart_Create(numSamples, numVars, numAxes);

	return CMD_RES_OK;
}
static commandResult_t CMD_Chart_SetVar(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() <= 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int varIndex = Tokenizer_GetArgInteger(0);
	if (varIndex >= g_chart->numVars){
//		ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set var %i, only %i vars defined (starting with 0)!", varIndex, g_chart->numVars);
		ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set var %i, only var %s%i defined!", varIndex, g_chart->numVars>1? "0-":"",g_chart->numVars-1);
		return CMD_RES_BAD_ARGUMENT;
	}
	const char *displayName = Tokenizer_GetArg(1);
	const char *axis = Tokenizer_GetArg(2);

	Chart_SetVar(g_chart, varIndex, displayName, axis);

	return CMD_RES_OK;
}
static commandResult_t CMD_Chart_SetAxis(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() <= 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int axisIndex = Tokenizer_GetArgInteger(0);
	if (axisIndex >= g_chart->numAxes){
//		ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set axis %i, only %i axes defined (starting with 0)!", axisIndex, g_chart->numAxes);
		ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set axis %i, only axis %s%i defined!", axisIndex, g_chart->numAxes>1? "0-":"", g_chart->numAxes-1);
		return CMD_RES_BAD_ARGUMENT;
	}
	const char *name = Tokenizer_GetArg(1);
	int cflags = Tokenizer_GetArgInteger(2);
	const char *label = Tokenizer_GetArg(3);

	Chart_SetAxis(g_chart, axisIndex, name, cflags, label);

	return CMD_RES_OK;
}
static commandResult_t CMD_Chart_AddNow(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	int cnt = Tokenizer_GetArgsCount();
	if (cnt < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	for (int i = 0; i < cnt; i++) {
		if (i >= g_chart->numVars){
//			ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set value for var %i, only %i vars defined (starting with 0)!", i, g_chart->numVars);
			ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set sample value for var %i, only var %s%i defined!", i, g_chart->numVars>1? "0-":"",g_chart->numVars-1);
		return CMD_RES_BAD_ARGUMENT;
		}

		float f = Tokenizer_GetArgFloat(i);
		Chart_SetSample(g_chart, i, f);
	}
	Chart_AddTime(g_chart, NTP_GetCurrentTimeWithoutOffset());  // Fix issue #1376 .....was NTP_GetCurrentTime() ... now "WithoutOffset" since NTP drivers timestamp are already offsetted

	return CMD_RES_OK;
}
static commandResult_t CMD_Chart_Add(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);

	int cnt = Tokenizer_GetArgsCount();
	if (cnt < 2) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	int time = Tokenizer_GetArgInteger(0);
	for (int i = 1; i < cnt; i++) {
		float f = Tokenizer_GetArgFloat(i);
		if (i > g_chart->numVars){
//			ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set value %f for var %i, only %i vars defined (starting with 0)!",f, i-1, g_chart->numVars);
			ADDLOG_ERROR(LOG_FEATURE_CMD, "Can't set value %f for var %i, only var %s%i defined!",f, i-1,  g_chart->numVars>1? "0-":"",g_chart->numVars-1);
			bk_printf("CHARTS: Can't set value %f for var %i, only var %s%i defined!\n",f, i-1,  g_chart->numVars>1? "0-":"",g_chart->numVars-1);
		return CMD_RES_BAD_ARGUMENT;
		}
		Chart_SetSample(g_chart, i - 1, f);
	}
	Chart_AddTime(g_chart, time);

	return CMD_RES_OK;
}

void DRV_Charts_Init() {



	//cmddetail:{"name":"chart_setAxis","args":"",
	//cmddetail:"descr":"Sets up an axis with a name, flags, and label. Currently flags can be 0 (left axis) or 1 (right axis). See [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_charts.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("chart_setAxis", CMD_Chart_SetAxis, NULL);
	//cmddetail:{"name":"chart_setVar","args":"",
	//cmddetail:"descr":"Associates a variable with a specific axis. See [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_charts.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("chart_setVar", CMD_Chart_SetVar, NULL);
	//cmddetail:{"name":"chart_create","args":"",
	//cmddetail:"descr":"Creates a chart with a specified number of samples, variables, and axes. See [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_charts.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("chart_create", CMD_Chart_Create, NULL);
	//cmddetail:{"name":"chart_addNow","args":"",
	//cmddetail:"descr":"Adds data to the chart using the current NTP time. See [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_charts.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("chart_addNow", CMD_Chart_AddNow, NULL);
	//cmddetail:{"name":"chart_add","args":"",
	//cmddetail:"descr":"Adds data to the chart with specified variables at a specific NTP time. See [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html).",
	//cmddetail:"fn":"NULL);","file":"driver/drv_charts.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("chart_add", CMD_Chart_Add, NULL);
}

