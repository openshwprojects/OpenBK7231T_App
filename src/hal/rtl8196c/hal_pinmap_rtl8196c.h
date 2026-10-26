#ifndef HAL_PINMAP_RTL8196C_H
#define HAL_PINMAP_RTL8196C_H

#ifdef PLATFORM_RTL8196C

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "gpio.h"

typedef struct {
    const char *name;
    int         gpio_id;
} rtl8196c_pin_t;

extern const rtl8196c_pin_t g_pins[PLATFORM_GPIO_MAX];

#endif /* PLATFORM_RTL8196C */
#endif /* HAL_PINMAP_RTL8196C_H */
