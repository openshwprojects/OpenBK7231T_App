
#include "../obk_config.h"

#include "drv_local.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"

int g_adr = 1;
int g_scl = 0;
int g_sda = 1;
softI2C_t g_scanI2C;

bool canUsePin(int pin) {
#if PLATFORM_ESP8266
	if (pin == 6) {
		return false;
	}
	if (pin == 7) {
		return false;
	}
#endif
	return true;
}
static int nextValidPin(int pin) {
	do {
		pin++;
		if (pin >= PLATFORM_GPIO_MAX)
			pin = 0;
	} while (!canUsePin(pin));
	return pin;
}
// startDriver MultiPinI2CScanner
void MultiPinI2CScanner_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	hprintf255(request, "Scan pins: %i %i, adr %i", g_scl, g_sda, g_adr);

}
void MultiPinI2CScanner_RunFrame() {

	g_scanI2C.pin_clk = g_scl;
	g_scanI2C.pin_data = g_sda;
	Soft_I2C_PreInit(&g_scanI2C);
	bool bOk = Soft_I2C_Start(&g_scanI2C, (g_adr << 1) + 0);
	Soft_I2C_Stop(&g_scanI2C);
	if (bOk) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "Pins SDA = %i, SCL =%i, adr 0x%x (dec %i)",
			g_sda, g_scl, g_adr);
	}
	g_adr++;
	if (g_adr >= 128) {
		g_adr = 0;

		// advance SCL
		g_scl = nextValidPin(g_scl);

		// avoid SDA == SCL
		if (g_scl == g_sda)
			g_scl = nextValidPin(g_scl);

		// if SCL wrapped back to start, move SDA
		if (g_scl == 0) {
			g_sda = nextValidPin(g_sda);

			// avoid SDA == SCL after moving SDA
			if (g_scl == g_sda)
				g_sda = nextValidPin(g_sda);
		}

		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "Next SDA = %i, SCL =%i", g_sda, g_scl);
	}
}

static commandResult_t CMD_FindSensorPower(const void* context, const char* cmd, const char* args, int cmdFlags);
static commandResult_t CMD_SuperScanner(const void* context, const char* cmd, const char* args, int cmdFlags);

void MultiPinI2CScanner_Init() {
	CMD_RegisterCommand("FindSensorPower", CMD_FindSensorPower, NULL);
	CMD_RegisterCommand("SuperScanner", CMD_SuperScanner, NULL);
}

static commandResult_t CMD_FindSensorPower(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int i, j;
	int sda, scl;
	uint8_t addrs[] = {0x38, 0x40, 0x44, 0x45};
	
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "Usage: FindSensorPower <sda_pin> <scl_pin>");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	sda = Tokenizer_GetArgInteger(0);
	scl = Tokenizer_GetArgInteger(1);
	
	softI2C_t scanI2C;
	scanI2C.pin_data = sda;
	scanI2C.pin_clk = scl;
	Soft_I2C_PreInit(&scanI2C);
	
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "Starting universal sensor power brute force on SDA=%i and SCL=%i...", sda, scl);
	
	for (i = 0; i < 30; i++) {
		if (i == scl || i == sda || i == 1 || i == 2) continue; // skip i2c pins and TX/RX
		
		HAL_PIN_Setup_Output(i);
		HAL_PIN_SetOutputValue(i, 1);
		rtos_delay_milliseconds(100);
		
		for (j = 0; j < 4; j++) {
			uint8_t addr = addrs[j] << 1;
			bool ack = Soft_I2C_Start(&scanI2C, addr);
			Soft_I2C_Stop(&scanI2C);
			
			if (ack) {
				// double check by reading
				Soft_I2C_Start(&scanI2C, addr | 1);
				uint8_t d1 = Soft_I2C_ReadByte(&scanI2C, false);
				Soft_I2C_Stop(&scanI2C);
				if (d1 != 0 && d1 != 0xFF) {
					addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SUCCESS! Sensor 0x%02X powered by GPIO %i (HIGH)! Data: %02X", addrs[j], i, d1);
				}
			}
		}
		
		HAL_PIN_Setup_Input(i);
		
		// Set pin LOW (Active-Low Power)
		HAL_PIN_Setup_Output(i);
		HAL_PIN_SetOutputValue(i, 0);
		rtos_delay_milliseconds(100);
		
		for (j = 0; j < 4; j++) {
			uint8_t addr = addrs[j] << 1;
			bool ack = Soft_I2C_Start(&scanI2C, addr);
			Soft_I2C_Stop(&scanI2C);
			
			if (ack) {
				// double check by reading
				Soft_I2C_Start(&scanI2C, addr | 1);
				uint8_t d1 = Soft_I2C_ReadByte(&scanI2C, false);
				Soft_I2C_Stop(&scanI2C);
				if (d1 != 0 && d1 != 0xFF) {
					addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SUCCESS! Sensor 0x%02X powered by GPIO %i (LOW)! Data: %02X", addrs[j], i, d1);
				}
			}
		}
		
		HAL_PIN_Setup_Input(i);
	}
	
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "Finished universal brute force test.");
	return CMD_RES_OK;
}

static commandResult_t CMD_SuperScanner(const void* context, const char* cmd, const char* args, int cmdFlags) {
    int pwr, sda, scl, j, state;
    softI2C_t scanI2C;
    uint8_t addrs[] = {0x38, 0x40, 0x44, 0x45};
    addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "Starting SUPER SCANNER. This will check 108,000 combinations. Wait 1 minute...");
    
    // state: 2=no power pin, 1=power pin high, 0=power pin low
    for (state = 2; state >= 0; state--) {
        for (pwr = -1; pwr < 30; pwr++) {
            if (pwr == 1 || pwr == 2) continue; // skip TX/RX
            if (state == 2 && pwr != -1) continue; // Only test pwr=-1 once
            if (state < 2 && pwr == -1) continue;
            
            if (pwr != -1) {
                HAL_PIN_Setup_Output(pwr);
                HAL_PIN_SetOutputValue(pwr, state);
                rtos_delay_milliseconds(50);
            }
            
            for (sda = 0; sda < 30; sda++) {
                if (sda == pwr || sda == 1 || sda == 2) continue;
                for (scl = 0; scl < 30; scl++) {
                    if (scl == pwr || scl == sda || scl == 1 || scl == 2) continue;
                    
                    scanI2C.pin_data = sda;
                    scanI2C.pin_clk = scl;
                    Soft_I2C_PreInit(&scanI2C);
                    
                    HAL_PIN_Setup_Input_Pullup(sda);
                    HAL_PIN_Setup_Input_Pullup(scl);
                    
                    if (HAL_PIN_ReadDigitalInput(sda) == 0) {
                        // Bus clear
                        HAL_PIN_Setup_Output(scl);
                        for(int k=0; k<18; k++) {
                            HAL_PIN_SetOutputValue(scl, k%2);
                            for(volatile int x=0; x<500; x++);
                        }
                        HAL_PIN_Setup_Input_Pullup(scl);
                    }
                    
                    if (HAL_PIN_ReadDigitalInput(sda) == 0 || HAL_PIN_ReadDigitalInput(scl) == 0) continue;
                    
                    for (j = 0; j < 4; j++) {
                        uint8_t addr = addrs[j] << 1;
                        bool ack = Soft_I2C_Start(&scanI2C, addr);
                        Soft_I2C_Stop(&scanI2C);
                        
                        if (ack) {
                            Soft_I2C_Start(&scanI2C, addr | 1);
                            uint8_t d1 = Soft_I2C_ReadByte(&scanI2C, false);
                            Soft_I2C_Stop(&scanI2C);
                            
                            if (d1 != 0 && d1 != 0xFF) {
                                if (pwr == -1) {
                                    addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SUCCESS! Sensor 0x%02X on SDA=%i, SCL=%i! (No pwr)", addrs[j], sda, scl);
                                } else {
                                    addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SUCCESS! Sensor 0x%02X on SDA=%i, SCL=%i! Pwr GPIO %i (%s)", addrs[j], sda, scl, pwr, state?"HIGH":"LOW");
                                }
                            }
                        }
                    }
                }
            }
            
            if (pwr != -1) {
                HAL_PIN_Setup_Input(pwr);
            }
        }
    }
    
    addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SUPER SCANNER FINISHED.");
    return CMD_RES_OK;
}
