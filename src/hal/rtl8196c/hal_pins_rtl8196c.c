#ifdef PLATFORM_RTL8196C

#include "hal_pinmap_rtl8196c.h"

const rtl8196c_pin_t g_pins[PLATFORM_GPIO_MAX] = {
    { "PA0",               GPIO_PIN('A', 0) },
    { "PA1",               GPIO_PIN('A', 1) },
    { "PA2 (Power LED)",   GPIO_PIN('A', 2) },
    { "PA3 (WPS Btn)",     GPIO_PIN('A', 3) },
    { "PA4 (USB Power)",   GPIO_PIN('A', 4) },
    { "PA5 (Reset Btn)",   GPIO_PIN('A', 5) },
    { "PA6 (WLAN LED)",    GPIO_PIN('A', 6) },
    { "PA7",               GPIO_PIN('A', 7) },
    { "PB0",               GPIO_PIN('B', 0) },
    { "PB1",               GPIO_PIN('B', 1) },
    { "PB2 (Port0 LED)",   GPIO_PIN('B', 2) },
    { "PB3 (Port1 LED)",   GPIO_PIN('B', 3) },
    { "PB4 (Port2 LED)",   GPIO_PIN('B', 4) },
    { "PB5 (Port3 LED)",   GPIO_PIN('B', 5) },
    { "PB6",               GPIO_PIN('B', 6) },
    { "PB7 (P1 LED)",      GPIO_PIN('B', 7) },
    { "PC0 (WAN LED)",     GPIO_PIN('C', 0) },
    { "PC1",               GPIO_PIN('C', 1) },
    { "PC2",               GPIO_PIN('C', 2) },
    { "PC3 (USB LED)",     GPIO_PIN('C', 3) },
    { "PC4",               GPIO_PIN('C', 4) },
    { "PC5",               GPIO_PIN('C', 5) },
    { "PC6 (Port4 LED)",   GPIO_PIN('C', 6) },
    { "PC7",               GPIO_PIN('C', 7) },
    { "PD0",               GPIO_PIN('D', 0) },
    { "PD1",               GPIO_PIN('D', 1) },
    { "PD2",               GPIO_PIN('D', 2) },
    { "PD3",               GPIO_PIN('D', 3) },
    { "PD4",               GPIO_PIN('D', 4) },
    { "PD5",               GPIO_PIN('D', 5) },
    { "PD6",               GPIO_PIN('D', 6) },
    { "PD7",               GPIO_PIN('D', 7) },
};

const char *HAL_PIN_GetPinNameAlias(int index)
{
    if (index < 0 || index >= PLATFORM_GPIO_MAX)
        return "error";
    return g_pins[index].name;
}

int HAL_PIN_Find(const char *name)
{
    if (!name || !*name)
        return -1;
    for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
        if (strncasecmp(g_pins[i].name, name, 3) == 0 &&
            (name[3] == 0 || name[3] == ' '))
            return i;
        if (strcasecmp(g_pins[i].name, name) == 0)
            return i;
    }
    return atoi(name);
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
    if (index < 0 || index >= PLATFORM_GPIO_MAX)
        return;
    gpio_write(g_pins[index].gpio_id, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
    if (index < 0 || index >= PLATFORM_GPIO_MAX)
        return 0;
    return gpio_read(g_pins[index].gpio_id);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
    if (index < 0 || index >= PLATFORM_GPIO_MAX)
        return;
    if (g_pins[index].gpio_id == GPIO_PIN('A', 4))
        return; /* Keep PA4 (USB VBUS Power) driven by USB host */
    gpio_set_dir(g_pins[index].gpio_id, 0);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
    HAL_PIN_Setup_Input_Pullup(index);
}

void HAL_PIN_Setup_Input(int index)
{
    HAL_PIN_Setup_Input_Pullup(index);
}

void HAL_PIN_Setup_Output(int index)
{
    if (index < 0 || index >= PLATFORM_GPIO_MAX)
        return;
    gpio_set_dir(g_pins[index].gpio_id, 1);
}

#endif /* PLATFORM_RTL8196C */
