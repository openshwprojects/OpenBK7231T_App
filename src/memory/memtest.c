#include "include.h"
#include "arm_arch.h"
#include "sys_rtos.h"

#include "../memory/memtest.h"
#include "../logging/logging.h"


#ifdef PLATFORM_BK7231T

static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
static size_t xBlockAllocatedBit = (1<<31);

int getMallocSize(void *ptr) {
	uint8_t *puc;
	BlockLink_t *pxLink;
	int presize, datasize;

    if (!ptr){
        return -2;
    }

    puc = ( uint8_t * ) ptr;

    puc -= xHeapStructSize;
	pxLink = ( void * ) puc;
	presize = (pxLink->xBlockSize & ~xBlockAllocatedBit);
	datasize = presize - xHeapStructSize;
    return datasize;
}

void logStack(const char *name){
	GETSTACK // macro which get a C variable SP and stacksize from memtest.h.
	int stacktotalsize = getMallocSize((void *)pxCurrentTCB->pxStack);
    uint8_t *stackstart = pxCurrentTCB->pxStack;
    uint8_t *stackend = stackstart + stacktotalsize;
    int used = ((uint32_t)stackend) - SP;

	ADDLOG_INFO(LOG_FEATURE_CMD, "%s:Stack: base:0x%08X use: %d/%d bytes", name, (uint32_t)pxCurrentTCB->pxStack, used, stacktotalsize);
}

void getStack(uint32_t* pTotal, uint32_t* pUsed){
	GETSTACK // macro which get a C variable SP and stacksize from memtest.h.
	int stacktotalsize = getMallocSize((void *)pxCurrentTCB->pxStack);
    uint8_t *stackstart = pxCurrentTCB->pxStack;
    uint8_t *stackend = stackstart + stacktotalsize;
    int used = ((uint32_t)stackend) - SP;
    if (pUsed) *pUsed = used;
    if (pTotal) *pTotal = stacktotalsize;
    return 0;
}


#else 
int getMallocSize(void *ptr){
    // unsupported
    return -1;
}
void logStack(const char *name){

}
void getStack(uint32_t* pTotal, uint32_t* pUsed){
    if (pUsed) *pUsed = 0;
    if (pTotal) *pTotal = 0;
    return -1;
}

#endif