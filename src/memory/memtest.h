/////////////////////////////////////////////////////////
// memtest.h
// a set of diagnostic routines to access 
// ARM stuff and FreeRtos stuff

///////////////////////////////////////////////////
// Notes on Debug Malloc testing for Beken T/N:
// in the application.mk file add:
//  CFLAGS += -DOBK_HEAPGUARD=1
//  LFLAGS += -Wl,-wrap,pvPortMalloc -Wl,-wrap,vPortFree
//  LFLAGS += -Wl,-wrap,pvPortRealloc
// This will enable wrapping of the malloc functions
// which increases each malloc by 32 bytes,
// recording caller of malloc and setting known guard bytes
// which are checked on free.


/////////////////////////////////////////////////////////////////////
// Duplicate some structures from freeRTOS so we can access them
// These are purely for testing!!!!

/////////////////////////////////////////////////////
// returns the size of a malloced pointer
// size will be the ALLOCATED size, not the REQUESTED size
// or -1 if unsupported platform.
// -2 if null
// -3 if ptr not in heap.
extern int getMallocSize(void *ptr);

/////////////////////////////////////////////////////
// log the current stack use.
// supply name to say where you logged it.
// will log task name and stack use as DEBUG
void logStack(const char *name);

///////////////////////////////////////////////////
// get the current stack size, use and TaskName
///////////////////////////////////////////////////
int getStack(uint32_t* pTotal, uint32_t* pUsed, const char **pName);

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
int stackCheck(int percentMax);

///////////////////////////////////////////////////
// TEST the heap.
// checks ALL heap entries for consistency.
// set logall to always get two lines of log.
///////////////////////////////////////////////////
void mallocTest(int logall);


#ifdef PLATFORM_BK7231T


/*
 * Task control block.  A task control block (TCB) is allocated for each task,
 * and stores task state information, including a pointer to the task's context
 * (the task's run time environment, including register values)
 */
typedef struct tskTaskControlBlock
{
	volatile StackType_t	*pxTopOfStack;	/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

	#if ( portUSING_MPU_WRAPPERS == 1 )
		xMPU_SETTINGS	xMPUSettings;		/*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
	#endif

	ListItem_t			xStateListItem;	/*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
	ListItem_t			xEventListItem;		/*< Used to reference a task from an event list. */
	UBaseType_t			uxPriority;			/*< The priority of the task.  0 is the lowest priority. */
	StackType_t			*pxStack;			/*< Points to the start of the stack. */
	char				pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

	#if ( portSTACK_GROWTH > 0 )
		StackType_t		*pxEndOfStack;		/*< Points to the end of the stack on architectures where the stack grows up from low memory. */
	#endif

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
		UBaseType_t		uxCriticalNesting;	/*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
	#endif

	#if ( configUSE_TRACE_FACILITY == 1 )
		UBaseType_t		uxTCBNumber;		/*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
		UBaseType_t		uxTaskNumber;		/*< Stores a number specifically for use by third party trace code. */
	#endif

	#if ( configUSE_MUTEXES == 1 )
		UBaseType_t		uxBasePriority;		/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
		UBaseType_t		uxMutexesHeld;
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		TaskHookFunction_t pxTaskTag;
	#endif

	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
		void *pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
	#endif

	#if( configGENERATE_RUN_TIME_STATS == 1 )
		uint32_t		ulRunTimeCounter;	/*< Stores the amount of time the task has spent in the Running state. */
	#endif

	#if ( configUSE_NEWLIB_REENTRANT == 1 )
		/* Allocate a Newlib reent structure that is specific to this task.
		Note Newlib support has been included by popular demand, but is not
		used by the FreeRTOS maintainers themselves.  FreeRTOS is not
		responsible for resulting newlib operation.  User must be familiar with
		newlib and must provide system-wide implementations of the necessary
		stubs. Be warned that (at the time of writing) the current newlib design
		implements a system-wide malloc() that must be provided with locks. */
		struct	_reent xNewLib_reent;
	#endif

	#if( configUSE_TASK_NOTIFICATIONS == 1 )
		volatile uint32_t ulNotifiedValue;
		volatile uint8_t ucNotifyState;
	#endif

	/* See the comments above the definition of
	tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE. */
	#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
		uint8_t	ucStaticallyAllocated; 		/*< Set to pdTRUE if the task is a statically allocated to ensure no attempt is made to free the memory. */
	#endif

	#if( INCLUDE_xTaskAbortDelay == 1 )
		uint8_t ucDelayAborted;
	#endif

} tskTCB;

/* The old tskTCB name is maintained above then typedefed to the new TCB_t name
below to enable the use of older kernel aware debuggers. */
typedef tskTCB TCB_t;

/*lint -e956 A manual analysis and inspection has been used to determine which
static variables must be declared volatile. */

extern PRIVILEGED_DATA TCB_t * volatile pxCurrentTCB;


// from heap_4.c
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

///////////////////////////////////////////////////////
// Are these generic to ALL our platforms?
//
// read the SP and create a local SP variable
#define GETSTACK \
	register uint32_t SP = 0; \
	__asm volatile ("mov %0, sp\n\t" : "=r" ( SP )	);

///////////////////////////////////////////////////////
// read the caller address for a function
// and create a local variable calleraddr
#define GETCALLERADDR \
    register uint32_t calleraddr = 0; \
    __asm volatile ("MOV %0, LR\n" : "=r" (calleraddr) ); 

#else 

// non-beken
#define GETSTACK \
	uint32_t SP = 0;

#define GETCALLERADDR \
    uint32_t calleraddr = 0;

#endif // en if T
