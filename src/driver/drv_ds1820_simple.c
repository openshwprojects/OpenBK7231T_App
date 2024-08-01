#include "drv_ds1820_simple.h"
#include <task.h>

static uint8_t   dsread=0;
static int Pin;
static int t=-127;
static int errcount=0;
static int lastconv;		// secondsElapsed on last successfull reading


// usleep adopted from DHT driver

void usleepds(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
#ifdef WIN32
	// not possible on Windows port
#elif PLATFORM_BL602 
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	//10
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#elif PLATFORM_W600
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#elif PLATFORM_W800
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	//10
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	//15
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	//20
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#elif PLATFORM_BEKEN
	usleep((17*r)/10);		// "1" is to fast and "2" to slow, 1.7 seems better than 1.5 (only from observing readings, no scope involved)
#elif PLATFORM_LN882H
	usleep(5*r);			// "5" seems o.k
#else
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#endif
}


/*

timing numbers and general code idea from

https://www.analog.com/en/resources/technical-articles/1wire-communication-through-software.html

Parameter 	Speed 		Recommended (µs)
A 		Standard 	6
B 		Standard 	64
C 		Standard 	60
D 		Standard 	10
E 		Standard 	9
F 		Standard 	55
G 		Standard 	0
H 		Standard 	480
I 		Standard 	70
J 		Standard 	410

*/

#define OWtimeA	6
#define OWtimeB	64
#define OWtimeC	60
#define OWtimeD	10
#define OWtimeE	9
#define OWtimeF	55
#define OWtimeG	0
#define OWtimeH	480
#define OWtimeI	70
#define OWtimeJ	410


int OWReset(int Pin)
{
        int result;

        usleepds(OWtimeG);
        HAL_PIN_Setup_Output(Pin);
        HAL_PIN_SetOutputValue(Pin,0); // Drives DQ low
        usleepds(OWtimeH);
        HAL_PIN_SetOutputValue(Pin,1); // Releases the bus
        usleepds(OWtimeI);
        HAL_PIN_Setup_Input(Pin);
        result = HAL_PIN_ReadDigitalInput(Pin) ^ 0x01; // Sample for presence pulse from slave
        usleepds(OWtimeJ); // Complete the reset sequence recovery
        return result; // Return sample presence pulse result
}

//-----------------------------------------------------------------------------
// Send a 1-Wire write bit. Provide 10us recovery time.
//
void OWWriteBit(int Pin, int bit)
{
		taskENTER_CRITICAL();
        if (bit)
        {
                // Write '1' bit
                HAL_PIN_Setup_Output(Pin);
                HAL_PIN_SetOutputValue(Pin,0); // Drives DQ low
                usleepds(OWtimeA);
                HAL_PIN_SetOutputValue(Pin,1); // Releases the bus
                usleepds(OWtimeB); // Complete the time slot and 10us recovery
        }
        else
        {
                // Write '0' bit
                HAL_PIN_Setup_Output(Pin);
                HAL_PIN_SetOutputValue(Pin,0); // Drives DQ low
                usleepds(OWtimeC);
                HAL_PIN_SetOutputValue(Pin,1); // Releases the bus
                usleepds(OWtimeD);
        }
        taskEXIT_CRITICAL();
}

//-----------------------------------------------------------------------------
// Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.
//
int OWReadBit(int Pin)
{
        int result;
		taskENTER_CRITICAL();
        HAL_PIN_Setup_Output(Pin);
        HAL_PIN_SetOutputValue(Pin,0); // Drives DQ low
        usleepds(OWtimeA);
        HAL_PIN_SetOutputValue(Pin,1); // Releases the bus
        usleepds(OWtimeE);
        HAL_PIN_Setup_Input(Pin);
//        result = HAL_PIN_ReadDigitalInput(Pin) ^ 0x01; // Sample for presence pulse from slave
        result = HAL_PIN_ReadDigitalInput(Pin); // Sample for presence pulse from slave
        usleepds(OWtimeF); // Complete the time slot and 10us recovery
        taskEXIT_CRITICAL();
        return result;
}

//-----------------------------------------------------------------------------
// Poll if DS1820 temperature conversion is complete
//
int DS1820TConversionDone(int Pin)
{
        int result;

        // Write '1' bit
        OWWriteBit(Pin, 1);
	// check for '1' - conversion complete (will be '0' else)
        return OWReadBit(Pin);
}


//-----------------------------------------------------------------------------
// Write 1-Wire data byte
//
void OWWriteByte(int Pin, int data)
{
        int loop;

        // Loop to write each bit in the byte, LS-bit first
        for (loop = 0; loop < 8; loop++)
        {
                OWWriteBit(Pin, data & 0x01);

                // shift the data byte for the next bit
                data >>= 1;
        }
}

//-----------------------------------------------------------------------------
// Read 1-Wire data byte and return it
//
int OWReadByte(int Pin)
{
        int loop, result=0;

        for (loop = 0; loop < 8; loop++)
        {
                // shift the result to get it ready for the next bit
                result >>= 1;

                // if result is one, then set MS bit
                if (OWReadBit(Pin))
                        result |= 0x80;
        }
        return result;
}

//-----------------------------------------------------------------------------
// Write a 1-Wire data byte and return the sampled result.
//
int OWTouchByte(int Pin, int data)
{
        int loop, result=0;

        for (loop = 0; loop < 8; loop++)
        {
                // shift the result to get it ready for the next bit
                result >>= 1;

                // If sending a '1' then read a bit else write a '0'
                if (data & 0x01)
                {
                        if (OWReadBit(Pin))
                                result |= 0x80;
                }
                else
                        OWWriteBit(Pin,0);

                // shift the data byte for the next bit
                data >>= 1;
        }
        return result;
}




uint8_t OWcrc( uint8_t *data, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inb = *data++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inb) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inb >>= 1;
		}
	}
	return crc;
}

// quicker CRC with lookup table
// based on code found here: https://community.st.com/t5/stm32-mcus-security/use-stm32-crc-to-compare-ds18b20-crc/m-p/333749/highlight/true#M4690
// Dallas 1-Wire CRC Test App -
//  x^8 + x^5 + x^4 + 1 0x8C (0x131)

uint8_t Crc8CQuick(uint8_t *Buffer,uint8_t Size)
{  static const uint8_t CrcTable[] = { // Nibble table for polynomial 0x8C
    0x00,0x9D,0x23,0xBE,0x46,0xDB,0x65,0xF8,
    0x8C,0x11,0xAF,0x32,0xCA,0x57,0xE9,0x74 };
    uint8_t Crc=0x00;
  while(Size--)
  {	
  	Crc ^= *Buffer++; // Apply Data    
  	Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F]; // Two rounds of 4-bits
  	Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F];  
  }
  return(Crc);

}



int DS1820_getTemp() {
	return t;
}


void DS1820_driver_Init(){};


void DS1820_AppendInformationToHTTPIndexPage(http_request_t *request)
{

        hprintf255(request, "<h5>DS1820 Temperature: %.2f C (read %i secs ago)</h5>",(float)t/100, g_secondsElapsed-lastconv);
}



void DS1820_OnEverySecond() {

	// for now just find the pin used
	//
	Pin=PIN_FindPinIndexForRole(IOR_DS1820_IO, 99);
	uint8_t scratchpad[9], crc;
	if (Pin != 99) {	// only if pin is set
		// request temp if conversion was requested two seconds after request
		// if (dsread == 1 && g_secondsElapsed % 5 == 2) {
		// better if we don't use parasitic power, we can check if conversion is ready		
		if (dsread == 1 && DS1820TConversionDone(Pin) == 1) {
		
			uint8_t Low=0,High=0,negative=0;
			int Val,Tc;

			if (OWReset(Pin) == 0) addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "DS1820 - Pin=%i  -- Reset failed",Pin); 
			OWWriteByte(Pin,0xCC);
			OWWriteByte(Pin,0xBE);

			for (int i = 0; i < 9; i++)
			  {
			    scratchpad[i] = OWReadByte(Pin);//read Scratchpad Memory of DS
			  }
//			crc= OWcrc(scratchpad, 8);
			crc= Crc8CQuick(scratchpad, 8);
			if (crc != scratchpad[8])
			{
				errcount++;
				addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "DS1820 - Read CRC=%x != calculated:%x (errcount=%i)\r\n",scratchpad[8],crc,errcount);
				addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "DS1820 - Scratchpad Data Read: %x %x %x %x %x %x %x %x %x \r\n",scratchpad[0],scratchpad[1],scratchpad[2],scratchpad[3],scratchpad[4],scratchpad[5],scratchpad[6],scratchpad[7],scratchpad[8]);
				
				if (errcount > 5) dsread=0;	// retry afer 5 failures		
			}	
			else
			{		
				Low = scratchpad[0];
				High = scratchpad[1];
			

				Val = (High << 8) + Low; // combine bytes to integer
				negative = (High >= 8); // negative temperature
				if (negative)
				{
				  Val = (Val ^ 0xffff) + 1;
				}
				// Temperature is returned in multiples of 1/16 °C
				//  we want a simple way to display e.g. xx.yy °C, so just multiply with 100 and we get xxyy 
				//  --> the last two digits will be the fractional part  (Val%100)
				// -->  the whole part of temperature is (int)Val/100
				// so we want 100/16 = 6.25 times the value (the sign to be able to show negative temperatures is in "factor") 
				Tc = (6 * Val) + Val / 4 ;
				t = negative ? -1 : 1 * Tc ;
				dsread=0;
				lastconv=g_secondsElapsed;
				addLogAdv(LOG_INFO, LOG_FEATURE_CFG, "DS1820 - Pin=%i temp=%s%i.%02i \r\n",Pin, negative ? "-" : "+", (int)Tc/100 , (int)Tc%100);
				addLogAdv(LOG_INFO, LOG_FEATURE_CFG, "DS1820 - High=%i Low=%i Val=%i Tc=%i  -- Read CRC=%x - calculated:%x \r\n",High, Low, Val,Tc,scratchpad[8],crc);
			}
		}
		else if (g_secondsElapsed % 5 == 0) {	// every 5 seconds
				// ask for "conversion" 

			if (OWReset(Pin) == 0){
				addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "DS1820 - Pin=%i  -- Reset failed",Pin);

				// if device is not found, maybe "usleep" is not working as expected
				// lets do usleepds() with numbers 50.000 and 100.00
				// if all is well, it should take 50ms and 100ms
				// if not, we need to "calibrate" the loop
				int tempsleep=5000;
				TickType_t actTick=portTICK_RATE_MS*xTaskGetTickCount();
				usleepds(tempsleep);
				int duration = (int)(portTICK_RATE_MS*xTaskGetTickCount()-actTick);

				addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "DS1820 - usleepds(%i) took %i ms ",tempsleep,duration);



				tempsleep=100000;
				actTick=portTICK_RATE_MS*xTaskGetTickCount();
				usleepds(tempsleep);
				duration = (int)(portTICK_RATE_MS*xTaskGetTickCount()-actTick);

				addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "DS1820 - usleepds(%i) took %i ms ",tempsleep,duration);
	
				if (duration < 95 || duration > 105){
					// calc a new factor for usleepds
					addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "usleepds duration divergates - proposed factor to adjust usleepds %f ",(float)100/duration);
				}
				
			} 
			else {
				OWWriteByte(Pin,0xCC);
				OWWriteByte(Pin,0x44);
				addLogAdv(LOG_INFO, LOG_FEATURE_CFG, "DS1820 - asked for conversion - Pin %i",Pin);
				dsread=1;
				errcount=0;
			}
		}

	
	}
}
