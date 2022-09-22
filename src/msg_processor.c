#include <stdlib.h>
#include <stdio.h>

#include "msg_processor.h"
#include "cJSON_N.h"
#include "new_cfg.h"
#include "new_pins.h"
#include "logging/logging.h"
#include "new_common.h"
#include "mqtt/new_mqtt.h"
#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_MAX 100
extern mainConfig_t g_cfg;
extern smartblub_config_data_t smartblub_config_data;
typedef enum {
  SSID,
  PSWD,
  MUNAME,
  MPSWD,
  MPORT,
  MURL,
  R_LED,
  R_BRIGHTNESS,
  G_LED,
  G_BRIGHTNESS,
  B_LED,
  B_BRIGHTNESS,
  W_LED,
  W_BRIGHTNESS,
  MTOPIC,
  RED,
  GREEN,
  WHITE,
  BLUE,
  LED,



    end
} TWKEYS;
const static struct {
  TWKEYS val;
  const char *str;

}

conversion[] = {
    {SSID, "SSID"},
    {PSWD, "PSWD"},
	{MUNAME,"MUNAME"},
	{MPSWD,"MPSWD"},
	{MPORT,"MPORT"},
	{MURL,"MURL"},
	{R_LED,"R_LED"},
	{R_BRIGHTNESS,"R_BRIGHTNESS"},
	{G_LED,"G_LED"},
	{G_BRIGHTNESS,"G_BRIGHTNESS"},
	{B_LED,"B_LED"},
	{B_BRIGHTNESS,"B_BRIGHTNESS"},
	{W_LED,"W_LED"},
	{W_BRIGHTNESS,"W_BRIGHTNESS"},
	{MTOPIC,"MTOPIC"},
	{RED,"RED"},
	{GREEN,"GREEN"},
	{WHITE,"WHITE"},
	{BLUE,"BLUE"},
	{LED,"LED"},




};
TWKEYS str2enum(const char *);

TWKEYS str2enum(const char *str) {
  int j;
  int data = end;
  //if(str!=NULL)
  //{
  for (j = 0; j < end; j++) {
    if (!strcmp(str, conversion[j].str)) {
      data = conversion[j].val;
      break;
    }
    //}
  }
  return data;
}
#define BUFSIZE		(64 * 1024 * 1024)

void print_json_value(const json_value_t *val, char *name,int depth);

void print_json_object(const json_object_t *obj, char * namee,int depth)
{



//	const char *name;
//	const json_value_t *val;
//	int n = 0;
//	int i;
//
//	//printf("{\n");
//	json_object_for_each(name, val, obj)
//	{
//		if (n != 0)
//			//printf(",\n");
//		n++;
//		for (i = 0; i < depth + 1; i++)
//			//printf("    ");
//		//printf("\"%s\": ", name);
//        addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"\"%s\": ",name);
//		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"value=%d\n",val);
//		print_json_value(val, depth + 1);
//	}
//
//	//printf("\n");
//	for (i = 0; i < depth; i++)
//	//	printf("    ");
//	//printf("}");
//		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"    ");
//	addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"}");
		const char *name;
		const json_value_t *val;
	json_object_for_each(name, val, obj)
	{
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"\"%s\": ",name);
		for (int i = 0; i < depth + 1; i++)
			print_json_value(val, name,depth + 1);
	}
}

void print_json_array(const json_array_t *arr, char  *name)
{
//{
//	const json_value_t *val;
//	int n = 0;
//	int i;
//
//	//printf("[\n");
//	json_array_for_each(val, arr)
//	{
//		if (n != 0)
//			//printf(",\n");
//		n++;
////		for (i = 0; i < depth + 1; i++)
//		//	printf("    ");
//
//		print_json_value(val, depth + 1);
//	}
//
////	printf("\n");
////	for (i = 0; i < depth; i++)
//		//printf("    ");
//	//printf("]");
}

void print_json_string(const char *str,char *namee)
{
	 switch (str2enum(namee)) {
	  case SSID: {
	    char *ssid;
	    ssid = str;
	   // g_wifi_ssid[64]=ssid;
	   // bk_printf("ssid->%s\n",ssid);

	    strcpy(smartblub_config_data.blub_wifi_ssid, ssid);
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"value=%s\n",ssid);
	  }
	  break;

	  case PSWD: {
	    char *pswd;
	    pswd = str;
	    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"value=%s\n",pswd);
		strcpy(smartblub_config_data.blub_wifi_pass, pswd);
	    //g_wifi_pass[64]=pswd;
	  //  bk_printf("pswd->%s\n",pswd);




	  }
	  break;
	  case MURL: {
	        char *murl;
	        murl = str;
	       // g_mqtt_host[64]=murl;
	       // bk_printf("murl->%s\n",murl);
	     //   strcpy_safe(g_mqtt_host, murl, sizeof(g_mqtt_host));
	    	strcpy(smartblub_config_data.blub_mqtt_host,murl);


	      }
	      break;
	    case MUNAME: {
	        char *muname;
	        muname = str;
	        bk_printf("muname->%s\n",muname);
	        strcpy(smartblub_config_data.blub_mqtt_userName,muname);
	       // g_mqtt_userName[64]=muname;


	      }
	      break;
	    case MPSWD: {
	        char *mpswd;
	        mpswd = str;
	      //  bk_printf("mpswd->%s\n",mpswd);
	        strcpy(smartblub_config_data.blub_mqtt_pass,mpswd);
	       // g_mqtt_pass[128]=mpswd;


	      }
	      break;
	    case MTOPIC: {
	         char *mtopic;
	         mtopic = str;
	       //  bk_printf("mpswd->%s\n",mpswd);
	         strcpy(smartblub_config_data.blub_mqtt_topic,mtopic);
	        // g_mqtt_pass[128]=mpswd;


	       }
	       break;
	  case LED: {
	 	    char *led;
	 	   led = str;
	 	    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"value=%s\n",str);
	 	   if (strncmp(led, "OFFALL", 6) == 0)

	 	  	    {
	 		      smartblub_config_data.led_offall=true;
	 	  	    }
	 	    //g_wifi_pass[64]=pswd;
	 	  //  bk_printf("pswd->%s\n",pswd);




	 	  }
	 	  break;
	 }
}

void print_json_number(json_int_t number,char *namee)
{

	switch (str2enum(namee)) {
					case RED:
						 if(number>=BRIGHTNESS_MIN && number <=BRIGHTNESS_MAX)
						 {
						addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"NUMBER=%" PRIu64 "\n",number);
							addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"in number\"%s\": ",namee);
							smartblub_config_data.red_led=true;
							 smartblub_config_data.r_brightness=number;
							 smartblub_config_data.r_pin=8;
							 smartblub_config_data.r_channel=3;
						 }
		    break;
					case WHITE:
						if(number>=BRIGHTNESS_MIN && number <=BRIGHTNESS_MAX)
						{
						addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"NUMBER=%" PRIu64 "\n",number);
							addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"in number\"%s\": ",namee);
							smartblub_config_data.white_led=true;
				          smartblub_config_data.w_brightness=number;
				          smartblub_config_data.w_pin=7;
				          smartblub_config_data.w_channel=6;
						}
							    break;
					case BLUE:
						 if(number>=BRIGHTNESS_MIN && number <=BRIGHTNESS_MAX)
						 {
						addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"NUMBER=%" PRIu64 "\n",number);
							addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"in number\"%s\": ",namee);
							smartblub_config_data.blue_led=true;
						  	 smartblub_config_data.b_brightness=number;
						  	smartblub_config_data.b_pin=26;
						  	smartblub_config_data.b_channel=5;
						 }
							    break;
					case GREEN:
						 if(number>=BRIGHTNESS_MIN && number <=BRIGHTNESS_MAX)
						 {
						addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"NUMBER=%" PRIu64 "\n",number);
							addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"in number\"%s\": ",namee);
						 	 smartblub_config_data.green_led=true;
						     smartblub_config_data.g_brightness=number;
						     smartblub_config_data.g_pin=24;
						     						  	smartblub_config_data.g_channel=7;
						 }
												    break;
					case MPORT:
					addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"NUMBER=%" PRIu64 "\n",number);
				addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"in number\"%s\": ",namee);

				 smartblub_config_data.blub_mqtt_port=number;
																	    break;
	}

}

void print_json_value(const json_value_t *val, char *name,int depth  )
{
	switch (json_value_type(val))
	{
	case JSON_VALUE_STRING:
		print_json_string(json_value_string(val),name);
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"string\n");

		break;
	case JSON_VALUE_NUMBER:
		print_json_number(json_value_number(val),name);
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"nuber\n");
		break;
	case JSON_VALUE_OBJECT:
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"object\n");
		print_json_object(json_value_object(val), name,depth);
		break;
	case JSON_VALUE_ARRAY:
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"array\n");
		print_json_array(json_value_array(val), name);
		break;
	case JSON_VALUE_TRUE:
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"true\n");

		break;
	case JSON_VALUE_FALSE:
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"false\n");

		break;
	case JSON_VALUE_NULL:
		 addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"null\n");
		break;
	}
}


int twProcessConfig(const char *data) {
	  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"data=%s\n",data);
	json_value_t *val = json_value_parse(data);
		if (val)
		{

			print_json_value(val, "start" ,0);
			json_value_destroy(val);
		}
		else
			  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"invalid json\n");

		return 0;
}


