#include "drv_ds1820_simple.h"

static uint8_t   dsread=0;
static int Pin;
static int t=-127;


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
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");	// 5
	}
#elif PLATFORM_LN882H
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
	}
#else
	for (volatile int i = 0; i < r; i++) {
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
	}
#endif
}


/*
//static int usleepfact=3;

#ifdef WIN32
	// not possible on Windows port
#elif PLATFORM_BL602 
static int usleepfact = 13;
#elif PLATFORM_W600
static int usleepfact = 7;
#elif PLATFORM_W800
//static int usleepfact = 33;
static int usleepfact = 25;
#elif PLATFORM_BEKEN
//static int usleepfact = 6;
static int usleepfact = 3;
#elif PLATFORM_LN882H
//static int usleepfact = 16;
static int usleepfact = 10;
#else
static int usleepfact = 3;
#endif


void usleepds(int r) //delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
	for (volatile int i = 0; i < r*usleepfact; i++) {
//		__asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
		__asm__("nop\nnop\nnop\nnop\nnop");
	}


}

*/




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
}

//-----------------------------------------------------------------------------
// Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.
//
int OWReadBit(int Pin)
{
        int result;

        HAL_PIN_Setup_Output(Pin);
        HAL_PIN_SetOutputValue(Pin,0); // Drives DQ low
        usleepds(OWtimeA);
        HAL_PIN_SetOutputValue(Pin,1); // Releases the bus
        usleepds(OWtimeE);
        HAL_PIN_Setup_Input(Pin);
//        result = HAL_PIN_ReadDigitalInput(Pin) ^ 0x01; // Sample for presence pulse from slave
        result = HAL_PIN_ReadDigitalInput(Pin); // Sample for presence pulse from slave
        usleepds(OWtimeF); // Complete the time slot and 10us recovery
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


int DS1820_getTemp() {
	return t;
}


void DS1820_driver_Init(){};


void DS1820_AppendInformationToHTTPIndexPage(http_request_t *request)
{

        hprintf255(request, "<h5>DS1820 Temperature: %.2f C </h5>",(float)t/100);
}


void DS1820_OnEverySecond() {

	// for now just find the pin used
	//
	Pin=PIN_FindPinIndexForRole(IOR_DS1820_IO, 99);

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

			Low = OWReadByte(Pin);
			High = OWReadByte(Pin);


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
			addLogAdv(LOG_INFO, LOG_FEATURE_CFG, "DS1820 - Pin=%i temp=%s%i.%02i \r\n",Pin, negative ? "-" : "+", (int)Tc/100 , (int)Tc%100);
			addLogAdv(LOG_INFO, LOG_FEATURE_CFG, "DS1820 - High=%i Low=%i Val=%i Tc=%i \r\n",High, Low, Val,Tc);
		}
		else if (g_secondsElapsed % 5 == 0) {	// every 5 seconds
				// ask for "conversion" 

			if (OWReset(Pin) == 0){
				addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "DS1820 - Pin=%i  -- Reset failed",Pin);

				// if device is not found, maybe "usleep" is not working as expected
				// lets do usleepds() with numbers 50.000 and 100.00
				// if all is well, it should take 50ms and 100ms
				// if not, we need to "calibrate" the loop
				int tempsleep=50000;
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
			}
		}

	
	}
}
