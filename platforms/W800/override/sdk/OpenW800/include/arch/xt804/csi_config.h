#ifndef __CSI_CONFIG_H__
#define __CSI_CONFIG_H__
#define CONFIG_CHIP_SL04 1
#define CONFIG_KERNEL_FREERTOS 1
//#define CONFIG_KERNEL_NONE 1
#define CONFIG_HAVE_VIC 1
#define CONFIG_SEPARATE_IRQ_SP 1
#define CONFIG_ARCH_INTERRUPTSTACK 4096
#define CONFIG_IRQ_VECTOR_SIZE   256
#define USE_UART0_PRINT            1

/*for dsp function used*/
#define SAVE_VR_REGISTERS          1
/*for float function used*/
#define SAVE_HIGH_REGISTERS     1

#ifdef CONFIG_KERNEL_NONE
#define CONFIG_SYSTEM_SECURE  1
#endif

#endif
