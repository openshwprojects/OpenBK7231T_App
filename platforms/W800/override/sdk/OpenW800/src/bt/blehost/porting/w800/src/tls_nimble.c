
#include <assert.h>
#include "syscfg/syscfg.h"
#include "nimble/nimble_port.h"
#include "wm_mem.h"
#include "wm_osal.h"

static void *tls_host_task_stack_ptr = NULL;
static tls_os_task_t tls_host_task_hdl = NULL;

static void nimble_host_task(void *arg)
{
    nimble_port_run();
}

void tls_nimble_start(void)
{
    tls_host_task_stack_ptr = (void *)tls_mem_alloc(MYNEWT_VAL(OS_HS_STACK_SIZE) * sizeof(uint32_t));
    assert(tls_host_task_stack_ptr != NULL);
    tls_os_task_create(&tls_host_task_hdl, "bth",
                       nimble_host_task,
                       (void *)0,
                       (void *)tls_host_task_stack_ptr,
                       MYNEWT_VAL(OS_HS_STACK_SIZE)*sizeof(uint32_t),
                       MYNEWT_VAL(OS_HS_TASK_PRIO),
                       0);
}

static void free_host_task_stack()
{
    if(tls_host_task_stack_ptr) {
        tls_mem_free(tls_host_task_stack_ptr);
        tls_host_task_stack_ptr = NULL;
    }
}
void tls_nimble_stop(void)
{
    if(tls_host_task_hdl) {
        tls_os_task_del_by_task_handle(tls_host_task_hdl, free_host_task_stack);
    }
}