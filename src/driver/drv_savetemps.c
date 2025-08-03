#include "../obk_config.h"


#if ENABLE_DRIVER_SAVETEMPS
#include "../httpserver/new_http.h"
#include "../ringbuff32.h"
#include "../logging/logging.h"

// this converts to string
#define STR_(X) #X

// this makes sure the argument is expanded before converting to string
#define STR(X) STR_(X)

// how many enties in buffer?
#define SAVEMAX 500
// save every X seconds
#define SAVETEMPRATE 30
//
int savetemprate = SAVETEMPRATE;
extern float g_wifi_temperature;

RB32_t* g_temperature_rb;


uint8_t savetemperature(uint8_t t){
	RB_saveVal(g_temperature_rb, t);
}

// some helper functions 

// round a float to next 0.5 value:
//  5.24 -->  5.0
//  5.25 -->  5.5
//  5.76 -->  6.0
// same for negative values
// -5.24 --> -5.0
// -5.26 --> -5.5
// -5.76 --> -6.0
float round_to_5(float t) {
	int temp = t < 0 ? 2*t -0.5 : 2*t + 0.5;
	return (float)temp/2;
}


// store Temperature values (floats) between -15.0° and +110°
// in 0.5 steps as (uint8_t) integers
// no check if temp is below lowest value yet
// but this is prepared:
// -15° ist stored as 1
// so value 0 shuld be "below scale"
// same for max value of 110° (stored at 251):
// we can use 255 as "above scale"
uint8_t floatTemp2int(float val){
	return (uint8_t)(2*round_to_5(val)+31);
}

// reverse: return float from our temperature representation
float intTemp2float(uint8_t val){
	return (float)val/2-15.5;
}


// callback function to "print" content to http request ...
void RB_CB_DataAsFloatTemp(RBTYPE val, void *buff, char *concatstr) {
	http_request_t *request = (http_request_t *)buff;
	hprintf255(request,"%.2f%s", intTemp2float(val),concatstr);
}



const char delim[]=",";

void SAVETEMPS_driver_Init()
{
	if (g_temperature_rb) RBfree(g_temperature_rb);
	g_temperature_rb = RBinit(SAVEMAX); 
	savetemprate = Tokenizer_GetArgIntegerDefault(1, SAVETEMPRATE);
}

void SAVETEMPS_driver_Stop()
{
	if (g_temperature_rb) RBfree(g_temperature_rb);
}

void SAVETEMPS_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState){

		char js[]="<script>const gwi=1e3,gh=500,lm=50;function scalegraph(t){svg=document.getElementById(\"mygraph\"),svg.setAttribute(\"width\",gwi*t),svg.setAttribute(\"heigth\",gh*t)}function md(t,e=\"a\"){return new Date(t).toLocaleString().replace(\"a\"==e?/,/:\"d\"==e?/,.*/:/.*, /,\"a\"==e?\"\\&#13;\":\"\")}function draw(){var t=document.getElementById(\"mygraph\"),e=document.getElementById(\"graphdata\").value.split(/\s*,\s*/).map((t=>isNaN(parseInt(t))?t:parseFloat(t))),n=5*(1+~~(Math.max(...e)/5)),a=(new Date).getTime();dd=a-e.length*ds;for(var r=\"' fill='transparent' stroke='black'>\",g=\"<line x1='0' x2='\",d=\"\",l=\"\",i=\"<path d='\",h=0;h<e.length;h++)x=~~(gwi/e.length*h),y=~~(gh-gh/n*e[h]),d+=\"<circle cx='\"+x+\"' cy='\"+y+\"' r='7' fill='#5cceee'><title> \"+e[h].toFixed(2)+\"°C &#13; \"+md(dd)+\"</title></circle>\",i+=h>0?\" L \":\"M \",i+=x+\" \"+y,h%~~(e.length/5)==0&&(l+=\"<text  transform='translate(\"+x+\", 525) rotate(-45)' text-anchor='end'>\"+md(dd,\"t\")+\"</text>\"),dd+=ds;for(i+=r+\"</path>\",l+=g+\"0' y1='0' y2='\"+gh+r+\"</line><text font-size='30px' y='-25' x='-25'>°C</text>\",l+=g+gwi+\"' y1='\"+\"500' y2='\"+gh+r+\"</line> \",h=0;h<5;h++)l+=\"<text  x='-50' y='\"+h*gh/5+\"'>\"+(n-h*n/5)+\"</text>\";t.innerHTML=d+i+l}ds=1e3*" STR(SAVETEMPRATE)" ;function updategraph(){draw(document.getElementById(\"mygraph\"),document.getElementById(\"graphdata\").value.split(/\s*,\s*/).map(Number))};</script>";
		poststr(request, js);
		
		hprintf255(request, "<p><svg xmlns='http://www.w3.org/2000/svg' id='mygraph' width='800' heigth='400' viewBox='-75 -75 1100 650' style='background: white'></svg>");
		hprintf255(request, "<p>Scale graph <input type='range' min='0.5' max='3' step='0.1' value='.8' onchange='scalegraph(this.value)'>");

	}
	else {
		hprintf255(request, "\n<input type='hidden' id='graphdata' value='");
		iterateRBtoBuff(g_temperature_rb, RB_CB_DataAsFloatTemp, request,",\0");
		hprintf255(request, "'><style onload='updategraph();'></style>\n");
		
	}
}



void SAVETEMPS_OnEverySecond()
{
	if (!(g_secondsElapsed % savetemprate)) {
		ADDLOG_INFO(LOG_FEATURE_RAW,"[saveTepm] g_wifi_temperature=%.2f  -- rouded: %.2f -- saving as %i\n", g_wifi_temperature,round_to_5(g_wifi_temperature),(uint8_t)(2*round_to_5(g_wifi_temperature)+31));
		savetemperature(floatTemp2int(g_wifi_temperature));
	}
}

#endif // ENABLE_DRIVER_SAVETEMPS



