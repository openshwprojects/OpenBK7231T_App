#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"

// startDriver TestCharts
void DRV_Test_Charts_AddToHtmlPage(http_request_t *request) {
	time_t timeStamps[16];
	float temperature[16];
	float humidity[16];
	char buffer[64];

	for (size_t i = 0; i < 16; i++) {
		timeStamps[i] = 1725606094 + i * 60;
		temperature[i] = 20.0 + (i % 5);
		humidity[i] = 80.0 + (i % 3);
	}
	poststr(request, "<canvas id=\"obkChart\" width=\"400\" height=\"200\"></canvas>");
	poststr(request, "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>");

	poststr(request, "<script>");
	poststr(request, "function cha() {");
	poststr(request, "console.log('Initializing chart');");
	poststr(request, "if (window.obkChartInstance) {");
	poststr(request, "    window.obkChartInstance.destroy();");
	poststr(request, "}");
	poststr(request, "var ctx = document.getElementById('obkChart').getContext('2d');");

	poststr(request, "var labels = [");
	for (size_t i = 0; i < 16; i++) {
		snprintf(buffer, sizeof(buffer), "new Date(%ld * 1000).toLocaleTimeString()", (long)(timeStamps[i]));
		poststr(request, buffer);
		if (i < 15) {
			poststr(request, ",");
		}
	}
	poststr(request, "];");

	poststr(request, "window.obkChartInstance = new Chart(ctx, {");
	poststr(request, "    type: 'line',");
	poststr(request, "    data: {");
	poststr(request, "        labels: labels,");
	poststr(request, "        datasets: [");
	poststr(request, "            {");
	poststr(request, "                label: 'Temperature',");
	poststr(request, "                data: [");
	for (size_t i = 0; i < 16; i++) {
		snprintf(buffer, sizeof(buffer), "%.2f", temperature[i]);
		poststr(request, buffer);
		if (i < 15) {
			poststr(request, ",");
		}
	}
	poststr(request, "],");
	poststr(request, "                borderColor: 'rgba(75, 192, 192, 1)',");
	poststr(request, "                yAxisID: 'y-temp',"); // Assign to temperature Y-axis
	poststr(request, "                fill: false");
	poststr(request, "            },");
	poststr(request, "            {");
	poststr(request, "                label: 'Humidity',");
	poststr(request, "                data: [");
	for (size_t i = 0; i < 16; i++) {
		snprintf(buffer, sizeof(buffer), "%.2f", humidity[i]);
		poststr(request, buffer);
		if (i < 15) {
			poststr(request, ",");
		}
	}
	poststr(request, "],");
	poststr(request, "                borderColor: 'rgba(232, 122, 432, 1)',");
	poststr(request, "                yAxisID: 'y-hum',"); // Assign to humidity Y-axis
	poststr(request, "                fill: false");
	poststr(request, "            }]");
	poststr(request, "    },");
	poststr(request, "    options: {");
	poststr(request, "        scales: {");
	poststr(request, "            x: {");
	poststr(request, "                type: 'category'");
	poststr(request, "            },");
	poststr(request, "            'y-temp': {"); // Define temperature Y-axis
	poststr(request, "                type: 'linear',");
	poststr(request, "                position: 'left',");
	poststr(request, "                title: {");
	poststr(request, "                    display: true,");
	poststr(request, "                    text: 'Temperature (°C)'");
	poststr(request, "                }");
	poststr(request, "            },");
	poststr(request, "            'y-hum': {"); // Define humidity Y-axis
	poststr(request, "                type: 'linear',");
	poststr(request, "                position: 'right',");
	poststr(request, "                title: {");
	poststr(request, "                    display: true,");
	poststr(request, "                    text: 'Humidity (%)'");
	poststr(request, "                },");
	poststr(request, "                grid: {");
	poststr(request, "                    drawOnChartArea: false"); // Avoid grid lines overlapping
	poststr(request, "                }");
	poststr(request, "            }");
	poststr(request, "        }");
	poststr(request, "    }");
	poststr(request, "});");
	poststr(request, "}");
	poststr(request, "</script>");
	poststr(request, "<style onload='cha();'></style>");
}

