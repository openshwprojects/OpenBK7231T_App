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
#define SF_RIGHT 1
typedef struct serie_s {
	char *title;
	float *samples;
	int flags;
} serie_t;

typedef struct series_s {
	int maxSamples;
	int nextSample;
	int lastSample;
	time_t *times;
	int numSerie;
	serie_t *serie;
} series_t;



void Series_Free(series_t *s) {
	if (!s) {
		return; 
	}
	for (int i = 0; i < s->numSerie; i++) {
		if (s->serie[i].title) {
			free(s->serie[i].title);
		}
		if (s->serie[i].samples) {
			free(s->serie[i].samples);
		}
	}
	free(s->times);
	free(s);
}
series_t *Series_Create(int maxSamples, int numVars) {
	series_t *s = (series_t *)malloc(sizeof(series_t));
	if (!s) {
		return NULL;
	}
	s->serie = (serie_t *)malloc(sizeof(serie_t) * numVars);
	if (!s->serie) {
		free(s);
		return NULL; 
	}
	s->times = (time_t *)malloc(sizeof(time_t) * maxSamples);
	if (!s->times) {
		free(s->serie);
		free(s);
		return NULL; 
	}
	for (int i = 0; i < numVars; i++) {
		s->serie[i].samples = (float*)malloc(sizeof(float) * maxSamples);
	}
	s->numSerie = numVars;
	s->maxSamples = maxSamples;
	s->nextSample = 0;
	s->lastSample = 0; 
	return s;
}
void Series_SetSerie(series_t *s, int idx, const char *title, int flags) {

	s->serie[idx].title = strdup(title);
	s->serie[idx].flags = flags;
}
void Series_SetSample(series_t *s, int idx, float value) {
	if (!s) {
		return;
	}
	s->serie[idx].samples[s->nextSample] = value;
}
void Series_AddTime(series_t *s, time_t time) {
	if (!s) {
		return;
	}
	s->times[s->nextSample] = time;
	s->nextSample = (s->nextSample + 1) % s->maxSamples;
	if (s->lastSample == s->nextSample) {
		s->lastSample = (s->lastSample + 1) % s->maxSamples;
	}
}
void Series_Iterate(series_t *s, int index, void (*callback)(float *val, time_t *time, void *userData), void *userData) {
	if (!s) {
		return;
	}
	int start = s->lastSample;
	int end = s->nextSample;

	if (start <= end) {
		for (int i = start; i < end; i++) {
			callback(&s->serie[index].samples[i], &s->times[i], userData);
		}
	} else {
		for (int i = start; i < s->maxSamples; i++) {
			callback(&s->serie[index].samples[i], &s->times[i], userData);
		}
		for (int i = 0; i < end; i++) {
			callback(&s->serie[index].samples[i], &s->times[i], userData);
		}
	}
}
void Series_DisplayLabel(float *val, time_t *time, void *userData) {
	char buffer[64];
	http_request_t *request = (http_request_t *)userData;
	if (request->userCounter != 0) {
		poststr(request, ",");
	}
	request->userCounter++;
	snprintf(buffer, sizeof(buffer), "new Date(%ld * 1000).toLocaleTimeString()", (long)(*time));
	poststr(request, buffer);
}
void Series_DisplayData(float *val, time_t *time, void *userData) {
	char buffer[32];
	http_request_t *request = (http_request_t *)userData;
	if (request->userCounter != 0) {
		poststr(request, ",");
	}
	request->userCounter++;
	snprintf(buffer, sizeof(buffer), "%.2f", *val);
	poststr(request, buffer);
}
void Series_Display(http_request_t *request, series_t *s) {
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
	Series_Iterate(s, 0, Series_DisplayLabel, request);
	poststr(request, "];");

	poststr(request, "window.myChartInstance = new Chart(ctx, {");
	poststr(request, "    type: 'line',");
	poststr(request, "    data: {");
	poststr(request, "        labels: labels,");  
	poststr(request, "        datasets: [");
	for (int i = 0; i < s->numSerie; i++) {
		if (i) {
			poststr(request, ",");
		}
		poststr(request, "{");
		hprintf255(request, "            label: '%s',", s->serie[i].title);
		poststr(request, "            data: [");
		request->userCounter = 0;
		Series_Iterate(s, i,  Series_DisplayData, request);
		poststr(request, "],");
		if (i == 1) {
			poststr(request, "                borderColor: 'rgba(232, 122, 432, 1)',");
		}
		else {
			poststr(request, "            borderColor: 'rgba(75, 192, 192, 1)',");
		}
		hprintf255(request, "                yAxisID: '%s',", s->serie[i].title);
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
	for (int i = 0; i < s->numSerie; i++) {
		poststr(request, ",");
		hprintf255(request, "            %s: {",  s->serie[i].title);
		if (s->serie[i].flags & SF_RIGHT) {
			poststr(request, "                position: 'right',");
		} else {
			poststr(request, "                position: 'left',");
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

	series_t *s = Series_Create(16, 2);
	Series_SetSerie(s, 0, "Temperature", 0);
	Series_SetSerie(s, 1, "Humidity", SF_RIGHT);

	Series_SetSample(s, 0, 20);
	Series_SetSample(s, 1, 89);
	Series_AddTime(s, 1725606094);
	Series_SetSample(s, 0, 22);
	Series_SetSample(s, 1, 88);
	Series_AddTime(s, 1725616094);
	Series_SetSample(s, 0, 26);
	Series_SetSample(s, 1, 91);
	Series_AddTime(s, 1725626094);
	Series_SetSample(s, 0, 30);
	Series_SetSample(s, 1, 92);
	Series_AddTime(s, 1725636094);
	Series_SetSample(s, 0, 28);
	Series_SetSample(s, 1, 92);
	Series_AddTime(s, 1725646094);
	Series_SetSample(s, 0, 27);
	Series_SetSample(s, 1, 91);
	Series_AddTime(s, 1725656094);
	Series_Display(request, s);
	Series_Free(s);
}


void DRV_Charts_Init() {


}

