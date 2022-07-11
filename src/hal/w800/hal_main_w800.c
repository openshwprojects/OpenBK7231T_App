
#include "../../new_common.h"
#include "../../logging/logging.h"

#include <string.h>
#include "wm_include.h"

#define LOG_FEATURE LOG_FEATURE_MAIN


#define  DEMO_TASK_PRIO			                32

#define    DEMO_TASK_SIZE      2048
static OS_STK 			DemoTaskStk[DEMO_TASK_SIZE];
#define DEMO_CONSOLE_BUF_SIZE   512

void user_main(void)
{
	Main_Init();

    tls_os_task_create(NULL, NULL,
    		Main_OnEverySecond,
                       NULL,
                       (void *)DemoTaskStk,          /* task's stack start address */
                       DEMO_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                       DEMO_TASK_PRIO,
                       0);
}


