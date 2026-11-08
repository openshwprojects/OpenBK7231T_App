#ifndef HAL_PINMAP_88PAAX01_H
#define HAL_PINMAP_88PAAX01_H

/*
 * Raw SoC GPIO mapping for Marvell 88PAAX01-BPK2 (HP DeskJet 2620):
 *  - P0..P31 : Hardware SoC GPIOs at GPIO_BASE (0xBD0B2000 + pin * 8)
 */
#define PLATFORM_GPIO_MAX 32

typedef struct pax_pin_s {
	const char *name;
	int         gpio;
} pax_pin_t;

extern pax_pin_t g_pins[];
extern const int g_numPins;

void pax_setup_default_channels(void);

#endif /* HAL_PINMAP_88PAAX01_H */
