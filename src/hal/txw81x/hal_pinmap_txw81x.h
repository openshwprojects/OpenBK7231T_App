#if PLATFORM_TXW81X

#include "txw81x/txw81x.h"
#include "txw81x/pin_names.h"
#include "txw81x/io_mask.h"
#include "txw81x/io_function.h"
#include "txw81x/adc_voltage_type.h"
#include "hal/gpio.h"

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"

typedef struct
{
	const char* name;
	enum pin_name pin;
	enum gpio_irq_event irq_mode;
} txwpin_t;

extern txwpin_t g_pins[];
extern int g_numPins;

#endif // PLATFORM_TXW81X
