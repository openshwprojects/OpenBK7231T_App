/////////////////////////////////////////////////////////
// memtest.c
// a set of diagnostic routines to access 
// ARM stuff and FreeRtos stuff
// Currently restricted to T in the ansence of other platforms to test on
//


#include "include.h"
#include "arm_arch.h"
#include "sys_rtos.h"
#include "../new_common.h"

#include "../memory/memtest.h"
#include "../logging/logging.h"


#ifdef PLATFORM_BK7231T

    ////////////////////////////////////////////////////////////
    // variables to mimic those used in mem_arch.c/heap_4.c
    static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
    static size_t xBlockAllocatedBit = (1<<31);

    extern uint8_t *ucHeap;
    extern unsigned char _empty_ram;

    #define HEAP_START_ADDRESS    (void*)&_empty_ram
    #define HEAP_END_ADDRESS      (void*)(0x00400000 + 256 * 1024)


    /////////////////////////////////////////////////////
    // returns the size of a malloced pointer
    // or -1 if unsupported platform.
    // -2 if null
    // -3 if ptr not in heap.
    int getMallocSize(void *ptr) {
        uint8_t *puc;
        BlockLink_t *pxLink;
        int presize, datasize;

        if (!ptr){
            return -2;
        }
        if ((((void *)ptr) < HEAP_START_ADDRESS) || (((void *)ptr) >= HEAP_END_ADDRESS)){
            return -3;
        }


        // test guard area
        uint8_t *orgptr = ptr;
    #ifdef OBK_HEAPGUARD
        orgptr -= 8;
    #endif
        puc = orgptr;
        puc -= xHeapStructSize;
        pxLink = ( BlockLink_t * ) puc;
        if (pxLink->pxNextFreeBlock){
            if ((((void *)pxLink->pxNextFreeBlock) < HEAP_START_ADDRESS) || (((void *)pxLink->pxNextFreeBlock) >= HEAP_END_ADDRESS)){
                // heap is broken?
                mallocTest(1);       
            }
        }

        presize = (pxLink->xBlockSize & ~xBlockAllocatedBit);
        datasize = presize - xHeapStructSize;
    #ifdef OBK_HEAPGUARD
        // we added 16 extra bytes.
        datasize -= 16;
    #endif
        return datasize;
    }

    /////////////////////////////////////////////////////
    // log the current stack use.
    // supply name to say where you logged it.
    // will log task name and stack use as DEBUG
    void logStack(const char *name){
        uint32_t Total;
        uint32_t Used; 
        const char *TaskName;
        getStack(&Total, &Used, &TaskName);
        ADDLOG_INFO(LOG_FEATURE_GENERAL, "%s:Stack: task:%s use: %d/%d bytes", name, TaskName, Used, Total);
    }


    ///////////////////////////////////////////////////
    // get the current stack size, use and TaskName
    ///////////////////////////////////////////////////
    static const char *notask = "NOTASK";
    static const char *badstack = "BADSTACK";
    int getStack(uint32_t* pTotal, uint32_t* pUsed, const char **pName){
        GETSTACK // macro which get a C variable SP and stacksize from memtest.h.
        if (pUsed) *pUsed = 0;
        if (pTotal) *pTotal = 0;
        if (pName) *pName = notask;

        if (!pxCurrentTCB){
            return -1;
        }
        uint8_t *stackstart = (uint8_t *)pxCurrentTCB->pxStack;

        // stacks are in the heap, check this
        if ((((void *)stackstart) < HEAP_START_ADDRESS) || (((void *)stackstart) >= HEAP_END_ADDRESS)){
            if (pName) *pName = badstack;
            return -2;
        }
        
        int stacktotalsize = getMallocSize((void *)pxCurrentTCB->pxStack);
        uint8_t *stackend = stackstart + stacktotalsize;
        int used = ((uint32_t)stackend) - SP;
        if (pUsed) *pUsed = used;
        if (pTotal) *pTotal = stacktotalsize;
        if (pName) *pName = pxCurrentTCB->pcTaskName;

        return 0;
    }


    ////////////////////////////////////////////////////
    // check that the stack is not fuller than percentMax
    // returns:
    // -1 - no thread
    // -2 out of bounds stack (corrupt?)
    // 0 - stack < percentMax
    // 1 - stack use > percentMax
    // usage:
    // if(stackCheck(80)) {
    //  log and return from function?
    // }
    int stackCheck(int percentMax){
        GETSTACK // macro which get a C variable SP and stacksize from memtest.h.
        if (!pxCurrentTCB){
            return -1;
        }
        uint8_t *stackstart = (uint8_t *)pxCurrentTCB->pxStack;

        // stacks are in the heap, check this
        if ((((void *)stackstart) < HEAP_START_ADDRESS) || (((void *)stackstart) >= HEAP_END_ADDRESS)){
            return -2;
        }
        
        int stacktotalsize = getMallocSize((void *)pxCurrentTCB->pxStack);
        uint8_t *stackend = stackstart + stacktotalsize;
        int used = ((uint32_t)stackend) - SP;
        if (used > (stacktotalsize/100)*percentMax){
            return 1;
        }
        return 0;
    }


    ///////////////////////////////////////////////////////////
    // scan the heap.
    // this checks all blocks (allocated and free) for consistency
    // set logall to get stats, otherwise it will only log errors
    ///////////////////////////////////////////////////////////
    void mallocTest(int logall){
        if (!ucHeap) {
            return;
        }
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
                    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: badblock next ptr 0x%08X",
                        (uint32_t)pxBlock->pxNextFreeBlock);
                    break;
                }
                if ((uint32_t)pxBlock->pxNextFreeBlock >= (uint32_t)pucHeapEnd){
                    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: badblock next ptr 0x%08X",
                        (uint32_t)pxBlock->pxNextFreeBlock);
                    break;
                }
            }

            pxBlock = (BlockLink_t *)((( uint8_t * )pxBlock) + size);

            if ((uint32_t)pxBlock >= (uint32_t)pucHeapEnd){
                ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: next off end 0x%08X",
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

        int errs = 0;

        if (!maxblocks){
            ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: #### got to maxblocks 5000!");
            errs++;
        }

        if (!maxblocks2){
            ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: #### FreeList got to maxblocks 5000!");
            errs++;
        }

        if((mallocfree != mallocfree2) || (countFree != countFree2)){
            ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: #### Freelist mismatch! count:%d != %d",
                countFree, countFree2
            );
            errs++;
        }

        // last block in free list should always be pxEnd with zero size
        if(pxFirstFree != pxBlock){
            ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: #### Freelist lastblock mismatch! %0x08X != %0x08X",
                pxFirstFree, pxBlock
            );
            errs++;
        }

        if(countAlloc2){
            ADDLOG_INFO(LOG_FEATURE_GENERAL, "MallocDiag: #### Freelist contains allocated blocks! count:%d",
                countAlloc2
            );
            errs++;
        }

        if ( (((uint32_t)pxBlock + sizeof(BlockLink_t)) - (uint32_t)pucAlignedHeap) != 
            ((uint32_t)HEAP_END_ADDRESS - (uint32_t)HEAP_START_ADDRESS) ){
            ADDLOG_ERROR(LOG_FEATURE_GENERAL, "MallocDiag: Scan incomplete?");
            errs++;
        }

        GETCALLERADDR
        char tmp[20];
        snprintf(tmp, sizeof(tmp), "malloctest from 0x%08X", (unsigned int) calleraddr);
        if (errs){
            logStack(tmp);
        }
        if (logall){
            ADDLOG_INFO(LOG_FEATURE_GENERAL, 
                "%s: First:0x%08X Last:0x%08X Scansize:0x%08X Expected: 0x%08X(%d)",
                tmp,
                (uint32_t)pucAlignedHeap, (uint32_t)pxBlock,
                ((uint32_t)pxBlock + sizeof(BlockLink_t)) - (uint32_t)pucAlignedHeap,
                ((uint32_t)HEAP_END_ADDRESS - (uint32_t)HEAP_START_ADDRESS),
                ((uint32_t)HEAP_END_ADDRESS - (uint32_t)HEAP_START_ADDRESS)
                );


            ADDLOG_INFO(LOG_FEATURE_GENERAL, 
                "%s: AllocCount:%d FreeCount:%d total:0x%08X used: 0x%08X largestfree: 0x%08x smallestfree: 0x%08X",
                tmp,
                countAllocated, countFree,
                totalmalloc, mallocused,
                largestfree, smallestfree
                );
        }
    }

    #ifdef OBK_HEAPGUARD

    extern void *__real_pvPortMalloc(size_t size);
    void *__wrap_pvPortMalloc(size_t size){
        #ifdef OBK_HEAPGUARD
            GETCALLERADDR
            // bk_printf("M\r\n"); - test to see if it hits here - it does.
            // add guard area - 8  bytes before and after allocated
            uint32_t newsize = size;
            if( ( newsize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
            {
                /* Byte alignment required. */
                newsize += ( portBYTE_ALIGNMENT - ( newsize & portBYTE_ALIGNMENT_MASK ) );
            }
            newsize += 16;
            void *p = __real_pvPortMalloc(newsize);
            void *orgptr = p;
            // Add 
            // calleraddr
            // AAAAAAAA
            // {malloc data}
            // AAAAAAAA
            // AAAAAAAA
            uint8_t* pStart = p;
            *(uint32_t*)pStart = calleraddr;
            pStart += 4;
            *(uint32_t*)pStart = 0xAAAAAAAA;
            pStart += 4;
            p = pStart;
            pStart = ((uint8_t*)orgptr) + (newsize - 8);
            *(uint32_t*)pStart = 0xAAAAAAAA;
            pStart += 4;
            *(uint32_t*)pStart = 0xAAAAAAAA;

            // NOTE: this is org malloc ptr + 8!!!
            return p;
        #else
            return __real_pvPortMalloc(size);
        #endif
    }

    extern void __real_vPortFree(void *pv);
    void __wrap_vPortFree(void *pv){
        #ifdef OBK_HEAPGUARD
            GETCALLERADDR
            uint8_t *orgptr = pv;
            if (orgptr){
                // test guard area
                orgptr -= 8;
                uint8_t* pStart = orgptr;
                uint8_t* puc = pStart;
                puc -= xHeapStructSize;
                BlockLink_t *pxLink = ( BlockLink_t * ) puc;
                uint32_t presize = (pxLink->xBlockSize & ~xBlockAllocatedBit);
                uint32_t datasize = presize - xHeapStructSize;
                uint32_t mcalleraddr = *(uint32_t*)pStart;
                int err = 0;
                pStart += 4;

                if ((*(uint32_t*)pStart) != 0xAAAAAAAA){
                    err++;
                }
                pStart += 4;
                pStart = ((uint8_t*)orgptr) + (datasize - 8);
                if ((*(uint32_t*)pStart) != 0xAAAAAAAA){
                    err++;
                }
                pStart += 4;
                if ((*(uint32_t*)pStart) != 0xAAAAAAAA){
                    err++;
                }
                if (err){
                    bk_printf("#####\r\n\r\nmalloc overwrite malloced:0x%08.8X freed:0x%08.8X\r\n\r\n#####\r\n", mcalleraddr, calleraddr);
                }
            }
            
            __real_vPortFree(orgptr);
        #else
            __real_vPortFree(ptr);
        #endif
    }

    extern void *__real_pvPortRealloc( void *pv, size_t xWantedSize );
    void *__wrap_pvPortRealloc( void *pv, size_t size ){
        #ifdef OBK_HEAPGUARD
            // test guard area

            uint8_t *orgptr = pv;
            orgptr -= 8;
            uint8_t* pStart = orgptr;
            uint8_t* puc = pStart;
            puc -= xHeapStructSize;
            BlockLink_t *pxLink = ( BlockLink_t * ) puc;
            uint32_t presize = (pxLink->xBlockSize & ~xBlockAllocatedBit);
            uint32_t datasize = presize - xHeapStructSize;
            uint32_t calleraddr = *(uint32_t*)pStart;
            int err = 0;
            pStart += 4;

            if (*(uint32_t*)pStart != 0xAAAAAAAA){
                err++;
            }
            pStart += 4;
            pStart = ((uint8_t*)orgptr) + (datasize - 8);
            if (*(uint32_t*)pStart != 0xAAAAAAAA){
                err++;
            }
            pStart += 4;
            if (*(uint32_t*)pStart != 0xAAAAAAAA){
                err++;
            }
            if (err){
                bk_printf("#####\r\n\r\nmalloc overwrite from 0x%08.8X\r\n\r\n#####\r\n", calleraddr);
            }

            uint32_t newsize = size;
            if( ( newsize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
            {
                /* Byte alignment required. */
                newsize += ( portBYTE_ALIGNMENT - ( newsize & portBYTE_ALIGNMENT_MASK ) );
            }
            newsize += 16;

            void *p = __real_pvPortRealloc((void *)orgptr, newsize);
            orgptr = p;
            // re-add guard area
            pStart = p;
            *(uint32_t*)pStart = calleraddr;
            pStart += 4;
            *(uint32_t*)pStart = 0xAAAAAAAA;
            pStart += 4;
            p = pStart;
            pStart = ((uint8_t*)orgptr) + (newsize - 8);
            *(uint32_t*)pStart = 0xAAAAAAAA;
            pStart += 4;
            *(uint32_t*)pStart = 0xAAAAAAAA;

            return p;
        #else
            return __real_pvPortRealloc(pv, xWantedSize);
        #endif
    }
    #endif


#else 
    int getMallocSize(void *ptr){
        // unsupported
        return -1;
    }
    void logStack(const char *name){
    }
    int getStack(uint32_t* pTotal, uint32_t* pUsed, const char **pName){
        if (pUsed) *pUsed = 0;
        if (pTotal) *pTotal = 0;
        if (pName) *pName = "unk";
        return -1;
    }
    int stackCheck(int percentMax){
        return 0;
    }
    void mallocTest(int logall){
    }

#endif


////////////////////////////////////
// test function 
// debug malloc - should cause malloc crash to be seen, but not kill system
// plain old malloc - may crash system
////////////////////////////////////

int CrashMalloc(){

    // overrun +-8 bytes
    uint8_t *ptr = malloc(1024);
    uint8_t *orgptr = ptr;
    ptr -= 8;
    for (int i = 0; i < 1024 + 8 + 8; i++){
        *(ptr++) = 0x55;
    }
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "CrashMalloc1 %X", *(uint32_t*)(orgptr-8));

    free(orgptr);

    // underrun -1 bytes
    ptr = malloc(1024);
    orgptr = ptr;
    ptr -= 1;
    for (int i = 0; i < 1024 + 1; i++){
        *(ptr++) = 0x55;
    }
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "CrashMalloc2 %X", *(uint32_t*)(orgptr-4));
    free(orgptr);

    // overrun 1 bytes
    ptr = malloc(1024);
    orgptr = ptr;
    for (int i = 0; i < 1024 + 1; i++){
        *(ptr++) = 0x55;
    }
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "CrashMalloc3 %X", *(uint32_t*)(orgptr+1024));
    free(orgptr);
    return 0;
}


// if stackcheck is implemented,
// then will not crash the system
int stackCrash(int level){
    char tmp[100];
    if (!stackCheck(80)){
        stackCrash(++level);
    } else {
        logStack("stackCrash");
        ADDLOG_INFO(LOG_FEATURE_GENERAL, "stackCrash got to level %d", level);
    }
    memset(tmp, 0, 100);
    return 0;
}
