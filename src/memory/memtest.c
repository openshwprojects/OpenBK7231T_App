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
    uint8_t *stackstart = (uint8_t *)pxCurrentTCB->pxStack;
    uint8_t *stackend = stackstart + stacktotalsize;
    int used = ((uint32_t)stackend) - SP;

	ADDLOG_INFO(LOG_FEATURE_CMD, "%s:Stack: base:0x%08X use: %d/%d bytes", name, (uint32_t)pxCurrentTCB->pxStack, used, stacktotalsize);
}

int getStack(uint32_t* pTotal, uint32_t* pUsed){
	GETSTACK // macro which get a C variable SP and stacksize from memtest.h.
	int stacktotalsize = getMallocSize((void *)pxCurrentTCB->pxStack);
    uint8_t *stackstart = (uint8_t *)pxCurrentTCB->pxStack;
    uint8_t *stackend = stackstart + stacktotalsize;
    int used = ((uint32_t)stackend) - SP;
    if (pUsed) *pUsed = used;
    if (pTotal) *pTotal = stacktotalsize;
    return 0;
}

uint8_t *ucHeap;
extern unsigned char _empty_ram;

#define HEAP_START_ADDRESS    (void*)&_empty_ram
#define HEAP_END_ADDRESS      (void*)(0x00400000 + 256 * 1024)

void mallocTest(){
    if (!ucHeap) return;
    size_t uxAddress = (size_t)ucHeap;
	if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
	{
		uxAddress += ( portBYTE_ALIGNMENT - 1 );
		uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
	}

	uint8_t *pucAlignedHeap = ( uint8_t * ) uxAddress;
	BlockLink_t *pxBlock = (BlockLink_t *)pucAlignedHeap;
    uint8_t *pucHeapEnd = HEAP_END_ADDRESS;
	BlockLink_t *pxFirstFree = NULL;
	BlockLink_t *pxLastFree = NULL;

    // walk all blocks noting if allocated or not
    int maxblocks = 5000;
    int countAllocated = 0;
    int countFree = 0;
    int totalmalloc = 0;
    int mallocfree = 0;
    int mallocused = 0;
    int largestfree = 0;
    int smallestfree = 0x80000000;

	vTaskSuspendAll();

    while (pxBlock && maxblocks){
        maxblocks--;
        int size = pxBlock->xBlockSize & ~xBlockAllocatedBit;

        if (size == 0){
            break;
        }

        if (pxBlock->xBlockSize & xBlockAllocatedBit){
            countAllocated++;
            mallocused += size;
        } else {
            if (!pxFirstFree){
                pxFirstFree = pxBlock;
            }
            countFree++;
            mallocfree += size;
            if (largestfree < size - sizeof(BlockLink_t)){
                largestfree = size - sizeof(BlockLink_t);
            }
            if (smallestfree > size - sizeof(BlockLink_t)){
                smallestfree = size - sizeof(BlockLink_t);
            }
        }
        totalmalloc += size;

        if (pxBlock->pxNextFreeBlock){
            if ((uint32_t)pxBlock->pxNextFreeBlock < (uint32_t)pucAlignedHeap){
                ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: badblock next ptr 0x%08X",
                    (uint32_t)pxBlock->pxNextFreeBlock);
                break;
            }
            if ((uint32_t)pxBlock->pxNextFreeBlock >= (uint32_t)pucHeapEnd){
                ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: badblock next ptr 0x%08X",
                    (uint32_t)pxBlock->pxNextFreeBlock);
                break;
            }
        }

        pxBlock = (BlockLink_t *)((( uint8_t * )pxBlock) + size);

        if ((uint32_t)pxBlock >= (uint32_t)pucHeapEnd){
            ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: next off end 0x%08X",
                (uint32_t)pxBlock);
            break;
        }

    }


    int countFree2 = 0;
    int countAlloc2 = 0;
    int mallocfree2 = 0;
    int maxblocks2 = 5000;

    while(pxFirstFree && maxblocks){
        maxblocks2--;
        int size = pxFirstFree->xBlockSize & ~xBlockAllocatedBit;
        if (size == 0){
            break;
        }

        if (pxFirstFree->xBlockSize & xBlockAllocatedBit){
            countAlloc2++;
        } else {
            countFree2++;
        }
        mallocfree2 += size;
        pxLastFree = pxFirstFree;
        pxFirstFree = pxFirstFree->pxNextFreeBlock;
    }

    ( void ) xTaskResumeAll();


    if (!maxblocks){
        ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: #### got to maxblocks 5000!");
    }

    if (!maxblocks2){
        ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: #### FreeList got to maxblocks 5000!");
    }

    if((mallocfree != mallocfree2) || (countFree != countFree2)){
        ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: #### Freelist mismatch! count:%d != %d",
            countFree, countFree2
        );
    }
    if(countAlloc2){
        ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: #### Freelist contains allocated blocks! count:%d",
            countAlloc2
        );
    }

    // last block in free list should always be pxEnd with zero size
    if(pxFirstFree != pxBlock){
        ADDLOG_INFO(LOG_FEATURE_CMD, "MallocDiag: #### Freelist lastbloack mismatch! %0x08X != %0x08X",
            pxFirstFree, pxBlock
        );
    }

	ADDLOG_INFO(LOG_FEATURE_CMD, 
        "MallocDiag: First:0x%08X Last:0x%08X Scansize:0x%08X Expected: 0x%08X(%d)",
        (uint32_t)pucAlignedHeap, (uint32_t)pxBlock,
        ((uint32_t)pxBlock + sizeof(BlockLink_t)) - (uint32_t)pucAlignedHeap,
        ((uint32_t)HEAP_END_ADDRESS - (uint32_t)HEAP_START_ADDRESS),
        ((uint32_t)HEAP_END_ADDRESS - (uint32_t)HEAP_START_ADDRESS)
        );

    if ( (((uint32_t)pxBlock + sizeof(BlockLink_t)) - (uint32_t)pucAlignedHeap) != 
         ((uint32_t)HEAP_END_ADDRESS - (uint32_t)HEAP_START_ADDRESS) ){
        ADDLOG_ERROR(LOG_FEATURE_CMD, "MallocDiag: Scan incomplete?");
    }

	ADDLOG_INFO(LOG_FEATURE_CMD, 
        "MallocDiag: AllocCount:%d FreeCount:%d total:0x%08X used: 0x%08X largestfree: 0x%08x smallestfree: 0x%08X",
        countAllocated, countFree,
        totalmalloc, mallocused,
        largestfree, smallestfree
        );


}


#else 
int getMallocSize(void *ptr){
    // unsupported
    return -1;
}
void logStack(const char *name){
}
int getStack(uint32_t* pTotal, uint32_t* pUsed){
    if (pUsed) *pUsed = 0;
    if (pTotal) *pTotal = 0;
    return -1;
}
void mallocTest(){
}

#endif