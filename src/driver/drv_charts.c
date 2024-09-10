#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"

/*
startDriver Charts

createSeries 0 Temperature

again:
// assumes that $CH1 is temperature_div10
addSample 0 $CH1*0.1
delay_s 10
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



void Chart_Free(chart_t *s) {
	if (!s) {
		return; 
	}
	for (int i = 0; i < s->numVars; i++) {
		if (s->vars[i].title) {
			free(s->vars[i].title);
		}
		if (s->vars[i].samples) {
			free(s->vars[i].samples);
		}
	}
	free(s->times);
	free(s);
}
chart_t *Chart_Create(int maxSamples, int numVars, int numAxes) {
	chart_t *s = (chart_t *)malloc(sizeof(chart_t));
	if (!s) {
		return NULL;
	}
	s->vars = (var_t *)malloc(sizeof(var_t) * numVars);
	if (!s->vars) {
		free(s);
		return NULL; 
	}
	s->axes = (axis_t *)malloc(sizeof(axis_t) * numAxes);
	if (!s->axes) {
		free(s);
		return NULL;
	}
	s->times = (time_t *)malloc(sizeof(time_t) * maxSamples);
	if (!s->times) {
		free(s->vars);
		free(s);
		return NULL; 
	}
	for (int i = 0; i < numVars; i++) {
		s->vars[i].samples = (float*)malloc(sizeof(float) * maxSamples);
	}
	s->numAxes = numAxes;
	s->numVars = numVars;
	s->maxSamples = maxSamples;
	s->nextSample = 0;
	s->lastSample = 0; 
	return s;
}
void Chart_SetAxis(chart_t *s, int idx, const char *name, int flags, const char *label) {

	s->axes[idx].name = strdup(name);
	s->axes[idx].label = strdup(label);
	s->axes[idx].flags = flags;
}
void Chart_SetVar(chart_t *s, int idx, const char *title, const char *axis) {

	s->vars[idx].title = strdup(title);
	s->vars[idx].axis = strdup(axis);
}
void Chart_SetSample(chart_t *s, int idx, float value) {
	if (!s) {
		return;
	}
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
	snprintf(buffer, sizeof(buffer), "new Date(%ld * 1000).toLocaleTimeString()", (long)(*time));
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

	poststr(request, "<canvas id=\"myChart\" width=\"400\" height=\"200\"></canvas>");
	poststr(request, "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>");

	poststr(request, "<script>");
	poststr(request, "function cha() {");
	poststr(request, "console.log('Initializing chart');");
	poststr(request, "if (window.myChartInstance) {");
	poststr(request, "    window.myChartInstance.destroy();");
	poststr(request, "}");
	poststr(request, "var ctx = document.getElementById('myChart').getContext('2d');");

	poststr(request, "var labels = [");
	request->userCounter = 0;
	Chart_Iterate(s, 0, Chart_DisplayLabel, request);
	poststr(request, "];");

	poststr(request, "window.myChartInstance = new Chart(ctx, {");
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
		poststr(request, "            data: [");
		request->userCounter = 0;
		Chart_Iterate(s, i,  Chart_DisplayData, request);
		poststr(request, "],");
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
	poststr(request, "});");
	poststr(request, "}");
	poststr(request, "</script>");
	poststr(request, "<style onload='cha();'></style>");

}
// startDriver Charts
void DRV_Charts_AddToHtmlPage(http_request_t *request) {

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
	Chart_Free(s);
}


void DRV_Charts_Init() {


}

