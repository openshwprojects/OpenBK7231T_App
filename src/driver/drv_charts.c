#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"

typedef struct sample_s {
	float sample;
	time_t time;
} sample_t;

typedef struct series_s {
	char *title;
	int maxSamples;
	int nextSample;
	int lastSample;
	sample_t *samples;
} series_t;

void Series_Free(series_t *s) {
	if (!s) {
		return; 
	}
	if (s->title) {
		free(s->title);
	}
	if (s->samples) {
		free(s->samples);
	}
	free(s);
}
series_t *Series_Create(const char *name, int maxSamples) {
	series_t *s = (series_t *)malloc(sizeof(series_t));
	if (!s) {
		return NULL;
	}
	s->title = strdup(name);
	if (!s->title) {
		free(s);
		return NULL; 
	}
	s->samples = (sample_t *)malloc(sizeof(sample_t) * maxSamples);
	if (!s->samples) {
		free(s->title);
		free(s);
		return NULL; 
	}
	s->maxSamples = maxSamples;
	s->nextSample = 0;
	s->lastSample = 0; 
	return s;
}
void Series_AddSample(series_t *s, time_t time, float value) {
	if (!s || !s->samples) {
		return;
	}
	s->samples[s->nextSample].time = time;
	s->samples[s->nextSample].sample = value;
	s->nextSample = (s->nextSample + 1) % s->maxSamples;
	if (s->lastSample == s->nextSample) {
		s->lastSample = (s->lastSample + 1) % s->maxSamples;
	}
}
void Series_Iterate(series_t *s, void (*callback)(sample_t *s, void *userData), void *userData) {
	if (!s || !s->samples) {
		return;
	}
	int start = s->lastSample;
	int end = s->nextSample;

	if (start <= end) {
		for (int i = start; i < end; i++) {
			callback(&s->samples[i], userData);
		}
	} else {
		for (int i = start; i < s->maxSamples; i++) {
			callback(&s->samples[i], userData);
		}
		for (int i = 0; i < end; i++) {
			callback(&s->samples[i], userData);
		}
	}
}
void Series_DisplayLabel(sample_t *v, void *userData) {
	char buffer[64];
	http_request_t *request = (http_request_t *)userData;
	if (request->userCounter != 0) {
		poststr(request, ",");
	}
	request->userCounter++;
	snprintf(buffer, sizeof(buffer), "new Date(%ld * 1000).toLocaleTimeString()", (long)(v->time));
	poststr(request, buffer);
}
void Series_DisplayData(sample_t *v, void *userData) {
	char buffer[32];
	http_request_t *request = (http_request_t *)userData;
	if (request->userCounter != 0) {
		poststr(request, ",");
	}
	request->userCounter++;
	snprintf(buffer, sizeof(buffer), "%.2f", v->sample);
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

	// Pre-format the labels manually using JavaScript's Date object
	poststr(request, "var labels = [");
	request->userCounter = 0;
	Series_Iterate(s, Series_DisplayLabel, request);
	poststr(request, "];");

	poststr(request, "window.myChartInstance = new Chart(ctx, {");
	poststr(request, "    type: 'line',");
	poststr(request, "    data: {");
	poststr(request, "        labels: labels,");  // Use the pre-formatted labels
	poststr(request, "        datasets: [{");
	hprintf255(request, "            label: '%s',",s->title);
	poststr(request, "            data: [");
	request->userCounter = 0;
	Series_Iterate(s, Series_DisplayData, request);
	poststr(request, "],");
	poststr(request, "            borderColor: 'rgba(75, 192, 192, 1)',");
	poststr(request, "            fill: false");
	poststr(request, "        }]");
	poststr(request, "    },");
	poststr(request, "    options: {");
	poststr(request, "        scales: {");
	poststr(request, "            x: {");
	poststr(request, "                type: 'category',");  // Use category scale instead of time scale
	poststr(request, "            },");
	poststr(request, "            y: {");
	poststr(request, "                beginAtZero: false");
	poststr(request, "            }");
	poststr(request, "        }");
	poststr(request, "    }");
	poststr(request, "});");
	poststr(request, "}");
	poststr(request, "</script>");
	poststr(request, "<style onload='cha();'></style>");

}
// startDriver Charts
void DRV_Charts_AddToHtmlPage(http_request_t *request) {

	series_t *s = Series_Create("Testperature", 16);
	Series_AddSample(s, 1725606094, 30);
	Series_AddSample(s, 1725616094, 34);
	Series_AddSample(s, 1725626094, 38);
	Series_AddSample(s, 1725636094, 27);
	Series_Display(request, s);
	Series_Free(s);
}


void DRV_Charts_Init() {


}

