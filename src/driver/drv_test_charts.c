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
	poststr(request, "<canvas id=\"myChart\" width=\"400\" height=\"200\"></canvas>");
	poststr(request, "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>");

	poststr(request, "<script>");
	poststr(request, "function cha() {");
	poststr(request, "console.log('qq');");
	poststr(request, "if (window.myChartInstance) {");
	poststr(request, "    window.myChartInstance.destroy();");
	poststr(request, "}");
	poststr(request, "var ctx = document.getElementById('myChart').getContext('2d');");
	poststr(request, "window.myChartInstance = new Chart(ctx, {");
	poststr(request, "    type: 'line',");
	poststr(request, "    data: {");
	poststr(request, "        labels: ['0s', '5s', '10s', '15s', '20s', '25s'],");
	poststr(request, "        datasets: [{");
	poststr(request, "            label: 'Temperature',");
	poststr(request, "            data: [22, 23, 21, 24, 25, 26],");
	poststr(request, "            borderColor: 'rgba(75, 192, 192, 1)',");
	poststr(request, "            fill: false");
	poststr(request, "        }]");
	poststr(request, "    },");
	poststr(request, "    options: {");
	poststr(request, "        scales: {");
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



