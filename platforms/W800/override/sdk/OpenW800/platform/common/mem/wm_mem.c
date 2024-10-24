/***************************************************************************** 
* 
* File Name : wm_mem.c
* 
* Description: memory manager Module 
* 
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-6-12 
*****************************************************************************/ 

#include "wm_osal.h"
#include "wm_mem.h"
#include "list.h"
#include <string.h>

extern u8 tls_get_isr_count(void);
/**
 * This variable is set if the memory mananger has been initialized.
 * This is available only for debug version of the driver
 */
bool         memory_manager_initialized = false;
/**
 * This mutex is used to synchronize the list of allocated
 * memory blocks. This is a debug version only feature
 */
tls_os_sem_t    *mem_sem;
#if WM_MEM_DEBUG


struct dl_list memory_used_list;
struct dl_list memory_free_list;
#define MEM_BLOCK_SIZE           800
MEMORY_BLOCK mem_blocks[MEM_BLOCK_SIZE];


u32 alloc_heap_mem_bytes = 0; 
u32 alloc_heap_mem_blk_cnt = 0;
u32 alloc_heap_mem_max_size = 0;

#define PRE_OVERSIZE        0
#define OVERSIZE        0

/**
 * This is a debug only function that performs memory management operations for us.
 * Memory allocated using this function is tracked, flagged when leaked, and caught for
 * overflows and underflows.
 *
 * \param size            The size in bytes of memory to
 *        allocate
 *
 * \param file            The full path of file where this
 *        function is invoked from
 * \param line            The line number in the file where this
 *        method was called from
 * \return Pointer to the allocated memory or NULL in case of a failure
 */
void * mem_alloc_debug(u32 size, char* file, int line)
{
    void *buf = NULL;
    u32 pad_len;
    int i =  0;
    u32  cpu_sr;
    //
    // If the memory manager has not been initialized, do so now
    //
    cpu_sr = tls_os_set_critical();
    if (!memory_manager_initialized) {
        tls_os_status_t os_status;
        memory_manager_initialized = true;
        //
        // NOTE: If two thread allocate the very first allocation simultaneously
        // it could cause double initialization of the memory manager. This is a
        // highly unlikely scenario and will occur in debug versions only.
        //
        os_status = tls_os_sem_create(&mem_sem, 1);
        if(os_status != TLS_OS_SUCCESS)
            printf("mem_alloc_debug: tls_os_sem_create mem_sem error\r\n");
        dl_list_init(&memory_used_list);
        dl_list_init(&memory_free_list);
        for(i = 0; i < MEM_BLOCK_SIZE; i++)
        {
            dl_list_add_tail(&memory_free_list, &mem_blocks[i].list);
        }
    }
    tls_os_release_critical(cpu_sr);

    tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    //
    // Allocate the required memory chunk plus header and trailer bytes
    //
    pad_len = sizeof(u32) - (size & 0x3);
    buf = malloc(sizeof(MEMORY_PATTERN) + PRE_OVERSIZE + size + pad_len + OVERSIZE + sizeof(MEMORY_PATTERN));

    if (buf) {
        //
        // Memory allocation succeeded. Add information about the allocated
        // block in the list that tracks all allocations.
        //
        PMEMORY_PATTERN  mem_ptn_hd;
        PMEMORY_PATTERN  mem_ptn_tl;
        PMEMORY_BLOCK  mem_blk_hd1;

	 if(dl_list_empty(&memory_free_list))
	 {
	     printf("Memory blocks empty!\r\n");
            free(buf);
            tls_os_release_critical(cpu_sr);
            tls_os_sem_release(mem_sem);
		 tls_mem_alloc_info();
            return NULL;
	 }
	 mem_blk_hd1 = dl_list_first(&memory_free_list, MEMORY_BLOCK, list);
	 dl_list_del(&mem_blk_hd1->list);
	 dl_list_add_tail(&memory_used_list, &mem_blk_hd1->list);
        alloc_heap_mem_bytes += size+sizeof(MEMORY_PATTERN)+sizeof(MEMORY_PATTERN)+pad_len + PRE_OVERSIZE + OVERSIZE;
        alloc_heap_mem_blk_cnt++;
        if (alloc_heap_mem_bytes > alloc_heap_mem_max_size)
        {
            alloc_heap_mem_max_size = alloc_heap_mem_bytes;
            //printf("alloc_heap_mem_max_size=%d\n", alloc_heap_mem_max_size);
        }
		
        mem_blk_hd1->pad = pad_len;
        mem_blk_hd1->file = file;
        mem_blk_hd1->line = line;
        mem_blk_hd1->length = size;
        mem_blk_hd1->header_pattern = (u32)buf;
		
        // Fill in the memory header and trailer
        mem_ptn_hd = (PMEMORY_PATTERN)buf;
        mem_ptn_hd->pattern0= MEM_HEADER_PATTERN;
        /*mem_ptn_hd->pattern1= MEM_HEADER_PATTERN;
        mem_ptn_hd->pattern2= MEM_HEADER_PATTERN;
        mem_ptn_hd->pattern3= MEM_HEADER_PATTERN;*/

        mem_ptn_tl = (PMEMORY_PATTERN)(((u8 *)(buf))+size + sizeof(MEMORY_PATTERN)+pad_len + PRE_OVERSIZE + OVERSIZE);
        mem_ptn_tl->pattern0= MEM_TAILER_PATTERN;
        /*mem_ptn_tl->pattern1= MEM_TAILER_PATTERN;
        mem_ptn_tl->pattern2= MEM_TAILER_PATTERN;
        mem_ptn_tl->pattern3= MEM_TAILER_PATTERN;*/

        // Jump ahead by memory header so pointer returned to caller points at the right place
        buf = ((u8 *)buf) + sizeof (MEMORY_PATTERN) + PRE_OVERSIZE;

#if 0

        printf("==>Memory was allocated from %s at line %d with length %d\n",
                  mem_blk_hd1->file,
                  mem_blk_hd1->line,               
                  mem_blk_hd1->length);
        printf("==>mem alloc ptr = 0x%x\n", buf);

#endif
    }
    else
    {
        printf("==>Memory was allocated from %s at line %d with length %d, allocated size %d, count %d\r\n",
                   file,
                   line,               
                   size, alloc_heap_mem_bytes, alloc_heap_mem_blk_cnt);
		tls_os_release_critical(cpu_sr);
    tls_os_sem_release(mem_sem);
	 tls_mem_alloc_info();
	 return buf;
    }
    tls_os_release_critical(cpu_sr);
    tls_os_sem_release(mem_sem);
    return buf;
}

void * mem_calloc_debug(u32 n, u32 size, char* file, int line)
{
    void *buf = NULL;
    u32 pad_len;
    int i =  0;
    u32  cpu_sr;
    //
    // If the memory manager has not been initialized, do so now
    //
    cpu_sr = tls_os_set_critical();
    if (!memory_manager_initialized) {
        tls_os_status_t os_status;
        memory_manager_initialized = true;
        //
        // NOTE: If two thread allocate the very first allocation simultaneously
        // it could cause double initialization of the memory manager. This is a
        // highly unlikely scenario and will occur in debug versions only.
        //
        os_status = tls_os_sem_create(&mem_sem, 1);
        if(os_status != TLS_OS_SUCCESS)
            printf("mem_alloc_debug: tls_os_sem_create mem_sem error\r\n");
        dl_list_init(&memory_used_list);
        dl_list_init(&memory_free_list);
        for(i = 0; i < MEM_BLOCK_SIZE; i++)
        {
            dl_list_add_tail(&memory_free_list, &mem_blocks[i].list);
        }
    }
    tls_os_release_critical(cpu_sr);

    tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    //
    // Allocate the required memory chunk plus header and trailer bytes
    //
    pad_len = sizeof(u32) - ((n*size) & 0x3);
    buf = malloc(sizeof(MEMORY_PATTERN) + PRE_OVERSIZE + n*size + pad_len + OVERSIZE + sizeof(MEMORY_PATTERN));

    if (buf) {
        //
        // Memory allocation succeeded. Add information about the allocated
        // block in the list that tracks all allocations.
        //
        PMEMORY_PATTERN  mem_ptn_hd;
        PMEMORY_PATTERN  mem_ptn_tl;
        PMEMORY_BLOCK  mem_blk_hd1;

	 if(dl_list_empty(&memory_free_list))
	 {
	     printf("Memory blocks empty!\r\n");
            free(buf);
            tls_os_release_critical(cpu_sr);
            tls_os_sem_release(mem_sem);
		 	tls_mem_alloc_info();
            return NULL;
	 }
	 mem_blk_hd1 = dl_list_first(&memory_free_list, MEMORY_BLOCK, list);
	 dl_list_del(&mem_blk_hd1->list);
	 dl_list_add_tail(&memory_used_list, &mem_blk_hd1->list);
        alloc_heap_mem_bytes += n*size+sizeof(MEMORY_PATTERN)+sizeof(MEMORY_PATTERN)+pad_len + PRE_OVERSIZE + OVERSIZE;
        alloc_heap_mem_blk_cnt++;
        if (alloc_heap_mem_bytes > alloc_heap_mem_max_size)
        {
            alloc_heap_mem_max_size = alloc_heap_mem_bytes;
            //printf("alloc_heap_mem_max_size=%d\n", alloc_heap_mem_max_size);
        }
		
        mem_blk_hd1->pad = pad_len;
        mem_blk_hd1->file = file;
        mem_blk_hd1->line = line;
        mem_blk_hd1->length = n*size;
        mem_blk_hd1->header_pattern = (u32)buf;
		
        // Fill in the memory header and trailer
        mem_ptn_hd = (PMEMORY_PATTERN)buf;
        mem_ptn_hd->pattern0= MEM_HEADER_PATTERN;
        /*mem_ptn_hd->pattern1= MEM_HEADER_PATTERN;
        mem_ptn_hd->pattern2= MEM_HEADER_PATTERN;
        mem_ptn_hd->pattern3= MEM_HEADER_PATTERN;*/

        mem_ptn_tl = (PMEMORY_PATTERN)(((u8 *)(buf))+n*size + sizeof(MEMORY_PATTERN)+pad_len + PRE_OVERSIZE + OVERSIZE);
        mem_ptn_tl->pattern0= MEM_TAILER_PATTERN;
        /*mem_ptn_tl->pattern1= MEM_TAILER_PATTERN;
        mem_ptn_tl->pattern2= MEM_TAILER_PATTERN;
        mem_ptn_tl->pattern3= MEM_TAILER_PATTERN;*/

        // Jump ahead by memory header so pointer returned to caller points at the right place
        buf = ((u8 *)buf) + sizeof (MEMORY_PATTERN) + PRE_OVERSIZE;

#if 0

        printf("==>Memory was allocated from %s at line %d with length %d\r\n",
                  mem_blk_hd1->file,
                  mem_blk_hd1->line,               
                  mem_blk_hd1->length);
        printf("==>mem alloc ptr = 0x%x\n", buf);

#endif
    }
    else
    {
        printf("==>Memory was allocated from %s at line %d with length %d, allocated size %d, count %d\r\n",
                   file,
                   line,               
                   n*size, alloc_heap_mem_bytes, alloc_heap_mem_blk_cnt);
		
    tls_os_release_critical(cpu_sr);
    tls_os_sem_release(mem_sem);
	 tls_mem_alloc_info();
	 return buf;
    }
    tls_os_release_critical(cpu_sr);
    tls_os_sem_release(mem_sem);
    return buf;
}
/**
 * This routine is called to free memory which was previously allocated using MpAllocateMemory function.
 * Before freeing the memory, this function checks and makes sure that no overflow or underflows have
 * happened and will also try to detect multiple frees of the same memory chunk.
 *
 * \param p    Pointer to allocated memory
 */
void mem_free_debug(void *p,  char* file, int line)
{
    PMEMORY_PATTERN  mem_ptn_hd;
    PMEMORY_PATTERN  mem_ptn_tl;
    PMEMORY_BLOCK  mem_blk_hd1;
    u8              needfree = 0;
	u8  haserr = 0;
    u32  cpu_sr;

    // Jump back by memory header size so we can get to the header
    mem_ptn_hd = (PMEMORY_PATTERN) (((u8 *)p) - sizeof(MEMORY_PATTERN)  - PRE_OVERSIZE);
    tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    dl_list_for_each(mem_blk_hd1, &memory_used_list, MEMORY_BLOCK, list){
        if(mem_blk_hd1->header_pattern == (u32)mem_ptn_hd)
        {
            needfree = 1;
            break;
        }
    }
    if(needfree)
    {
        dl_list_del(&mem_blk_hd1->list);
        dl_list_add_tail(&memory_free_list, &mem_blk_hd1->list);
        alloc_heap_mem_bytes -= mem_blk_hd1->length + sizeof(MEMORY_PATTERN) + sizeof(MEMORY_PATTERN) + PRE_OVERSIZE + OVERSIZE +
            mem_blk_hd1->pad;
        alloc_heap_mem_blk_cnt--;
    }
    if(needfree == 0)
    {
	 printf("Memory Block %p was deallocated from %s at line %d \r\n", mem_ptn_hd, file, line);
	 printf("Memory %p has been deallocated!\r\n", p);
	 dl_list_for_each_reverse(mem_blk_hd1, &memory_free_list, MEMORY_BLOCK, list){
            if(mem_blk_hd1->header_pattern == (u32)mem_ptn_hd)
            {
                printf("Memory Block %p has been put free list!\r\n", mem_ptn_hd);
                break;
            }
        }
        tls_os_release_critical(cpu_sr);
        tls_os_sem_release(mem_sem);
        tls_mem_alloc_info();
	 return;
    }
#if 0
    if(mem_blk_hd1->line == 976 || mem_blk_hd1->line == 983)
    {
	 printf("Memory Block %p can not deallocated from %s at line %d \r\n", mem_ptn_hd, file, line);
	 printf("Memory %p has been deallocated!\r\n", p);
        tls_mem_alloc_info();
    }
#endif
    mem_ptn_tl = (PMEMORY_PATTERN) ((u8 *)p + mem_blk_hd1->length + mem_blk_hd1->pad + OVERSIZE);
    //
    // Check that header was not corrupted
    //
    if (mem_ptn_hd->pattern0 != MEM_HEADER_PATTERN /*|| mem_ptn_hd->pattern1 != MEM_HEADER_PATTERN 
		|| mem_ptn_hd->pattern2 != MEM_HEADER_PATTERN || mem_ptn_hd->pattern3 != MEM_HEADER_PATTERN*/) 
    {
        printf("Memory %p was deallocated from %s at line %d \r\n", p, file, line);
        printf("Memory header corruption due to underflow detected at memory block %p\r\n",
 	            mem_ptn_hd);
        printf("Header pattern 0(0x%x)\r\n",//, 1(0x%x), 2(0x%x), 3(0x%x)
			mem_ptn_hd->pattern0/*,
			mem_ptn_hd->pattern1,
			mem_ptn_hd->pattern2,
			mem_ptn_hd->pattern3*/);
        //printf("Dumping information about memory block. "
        //        "This information may itself have been "
         //       "corrupted and could cause machine to bugcheck.\r\n");
        printf("Memory was allocated from %s at line %d with length %d\r\n",
                mem_blk_hd1->file,
                mem_blk_hd1->line,
                mem_blk_hd1->length);
        haserr = 1;
    }

#if 0
    printf("<==free memory allocated from %s at line %d with length %d\n",
            mem_blk_hd->file,
            mem_blk_hd->line,
            mem_blk_hd->length);
    printf("<==free memory 0x%x\n", (u8 *)mem_blk_hd+sizeof(*mem_blk_hd));
#endif

    //
    // Check that trailer was not corrupted
    //
    if(mem_ptn_tl->pattern0 != MEM_TAILER_PATTERN /*|| mem_ptn_tl->pattern1 != MEM_TAILER_PATTERN 
		|| mem_ptn_tl->pattern2 != MEM_TAILER_PATTERN || mem_ptn_tl->pattern3 != MEM_TAILER_PATTERN*/) {
	 printf("Memory %p was deallocated from %s at line %d \r\n", p, file, line);
        printf("Memory tailer corruption due to overflow detected at %p\r\n", mem_ptn_hd);
        printf("Tailer pattern 0(0x%x)\r\n",//, 1(0x%x), 2(0x%x), 3(0x%x)
			mem_ptn_tl->pattern0/*,
			mem_ptn_tl->pattern1,
			mem_ptn_tl->pattern2,
			mem_ptn_tl->pattern3*/);
        //printf("Dumping information about memory block. "
        //       "This information may itself have been "
        //        "corrupted and could cause machine to bugcheck.\r\n");
        printf("Memory was allocated from %s at line %d with length %d\r\n",
                mem_blk_hd1->file, mem_blk_hd1->line, mem_blk_hd1->length);
		haserr = 1;
    }
    if(needfree){
        free(mem_ptn_hd);
    }
    
    tls_os_release_critical(cpu_sr);
    tls_os_sem_release(mem_sem);

	if(haserr)
        tls_mem_alloc_info();
}

void * mem_realloc_debug(void *mem_address, u32 size, char* file, int line)
{
	void * mem_re_addr;
	u32 cpu_sr;

	if ((mem_re_addr = mem_alloc_debug(size,  file, line)) == NULL){
		printf("mem_realloc_debug failed(size=%d).\r\n", size);
		return NULL;
	}
	if(mem_address != NULL)
	{
	    cpu_sr = tls_os_set_critical();
		memcpy(mem_re_addr, mem_address, size);
		tls_os_release_critical(cpu_sr);
		mem_free_debug(mem_address, file, line);
	}
	//printf("mem_realloc_debug mem_address=%p, mem_re_addr=%p, size=%d, file=%s, line=%d\n", mem_address, mem_re_addr, size, file, line);
	return mem_re_addr;
}

void tls_mem_alloc_info(void)
{
    int i;
    MEMORY_BLOCK * pos;
    u32 cpu_sr;
	
	tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
	printf("==>Memory was allocated size %d, count %d\r\n",
		alloc_heap_mem_bytes, alloc_heap_mem_blk_cnt);
    i = 1;
    dl_list_for_each(pos, &memory_used_list, MEMORY_BLOCK, list){
        printf("Block(%2d): addr<%p>, file<%s>, line<%d>, length<%d>\r\n",
                i, pos->header_pattern, pos->file, pos->line, pos->length);
        i++;
    }
	tls_os_release_critical(cpu_sr);
    tls_os_sem_release(mem_sem);
    
}

int is_safe_addr_debug(void* p, u32 len, char* file, int line)
{
    int i;
    MEMORY_BLOCK * pos;
    u32 cpu_sr;

    if(((u32)p) >= (u32)0x64ae8 || ((u32)p) < (u32)0x54ae8)
    {
        return 1;
    }
    tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    i = 1;
    dl_list_for_each(pos, &memory_used_list, MEMORY_BLOCK, list){
        if((pos->header_pattern + sizeof (MEMORY_PATTERN)  + PRE_OVERSIZE) <= ((u32)p) && ((u32)p) <= ((u32)(pos->header_pattern + sizeof(MEMORY_PATTERN) + PRE_OVERSIZE + pos->length)))
        {
            if(((u32)p) + len > ((u32)(pos->header_pattern + sizeof(MEMORY_PATTERN) + PRE_OVERSIZE + pos->length)))
            {
                printf("==>Memory oversize. Block(%2d): addr<%p>, file<%s>, line<%d>, length<%d>\r\n",
                    i, pos->header_pattern, pos->file, pos->line, pos->length);
                break;
            }
            else
            {
                tls_os_release_critical(cpu_sr);
                tls_os_sem_release(mem_sem);
                return 1;
            }
        }
        //else if(((u32)p) < pos->header_pattern)
        //{
        //    //tls_os_release_critical(cpu_sr);
        //    tls_os_sem_release(mem_sem);
        //    return 1;
       // }
        i++;
    }
    tls_os_release_critical(cpu_sr);
    tls_os_sem_release(mem_sem);
    printf("==>Memory is not safe addr<%p>, file<%s>, line<%d>.\r\n",p, file, line);
    return 0;
}

#else /* WM_MEM_DEBUG */
#if TLS_OS_FREERTOS
u32 alloc_heap_mem_bytes = 0; 
u32 alloc_heap_mem_blk_cnt = 0;
u32 alloc_heap_mem_max_size = 0;
#define OS_MEM_FLAG  (0x5AA5A55A)
#define NON_OS_MEM_FLAG (0xA55A5A5A)
#define MEM_HEAD_FLAG (0xBB55B55B)
#endif

#define USING_ADD_HEADER   1
extern u32 total_mem_size;
void * mem_alloc_debug(u32 size)
{
//    u32 cpu_sr = 0;
    u32 *buffer = NULL;
	u32 length = size;


	//printf("size:%d\n", size);
    if (!memory_manager_initialized) {
        tls_os_status_t os_status;
        memory_manager_initialized = true;
        //
        // NOTE: If two thread allocate the very first allocation simultaneously
        // it could cause double initialization of the memory manager. This is a
        // highly unlikely scenario and will occur in debug versions only.
        //
        os_status = tls_os_sem_create(&mem_sem, 1);
        if(os_status != TLS_OS_SUCCESS)
            printf("mem_alloc_debug: tls_os_sem_create mem_sem error\r\n");
    }

#if USING_ADD_HEADER
    length += 8;

    if(tls_get_isr_count() > 0)
    {
        extern void *pvPortMalloc( size_t xWantedSize );
        buffer = pvPortMalloc(length);
		if(buffer) 
		{
			*buffer = OS_MEM_FLAG;
			buffer++;
			*buffer = length;
			buffer++;
		}
    }
    else
    {
    	tls_os_sem_acquire(mem_sem, 0);
        //cpu_sr = tls_os_set_critical();
        buffer = (u32*)malloc(length);
	    if(buffer) 
	    {
	        *buffer = NON_OS_MEM_FLAG;
	        buffer++;
			*buffer = length;
			buffer++;
			total_mem_size -= length;
	    }
	    //if(tls_get_isr_count() == 0)
	   // {
	      //  tls_os_release_critical(cpu_sr);	
			tls_os_sem_release(mem_sem);
	   // }
    }
#else
	tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    buffer = (u32*)malloc(length);
    tls_os_release_critical(cpu_sr); 
	tls_os_sem_release(mem_sem);
#endif
	return buffer;

}

void mem_free_debug(void *p)
{
//    u32 cpu_sr = 0;
//	u32 len = 0;
#if USING_ADD_HEADER
    u32* intMemPtr = NULL;
	u8 isrstatus = 0;

	isrstatus = tls_get_isr_count();
    if(isrstatus == 0)
    {
    	tls_os_sem_acquire(mem_sem, 0);
       // cpu_sr = tls_os_set_critical();
    }
	
	intMemPtr = (u32*)p;
    if(p)
    {
		intMemPtr -= 2;    
		if (*intMemPtr == OS_MEM_FLAG)
        {
			extern void vPortFree( void *pv );
			vPortFree(intMemPtr);
			intMemPtr = NULL;
        }
		else if (*intMemPtr == NON_OS_MEM_FLAG)
        {
			total_mem_size += *(intMemPtr + 1);
			free(intMemPtr);
			intMemPtr = NULL;
        }
        else
        {
			printf("mem_free_debug ptr error!!!!!\r\n");
		}
    }

    if(isrstatus == 0)
    {
   //     tls_os_release_critical(cpu_sr);	
		tls_os_sem_release(mem_sem);
    }
#else //UCOSII
	tls_os_sem_acquire(mem_sem, 0);
	cpu_sr = tls_os_set_critical();
	free(p);
	tls_os_release_critical(cpu_sr);	
	tls_os_sem_release(mem_sem);
#endif
}


void * mem_realloc_debug(void *mem_address, u32 size)
{
    u32 * mem_re_addr = NULL;
//    u32 cpu_sr = 0;
	u32 length = size;

#if USING_ADD_HEADER
    length = size + 2*4;

    if(tls_get_isr_count() > 0)
    {
		extern void *pvPortMalloc( size_t xWantedSize );
		mem_re_addr = pvPortMalloc(length);
		if (mem_re_addr)
		{
			return NULL;
		}
		if(mem_address != NULL)
		{
			if (*((u32 *)mem_address-1)> size)
			{
				memcpy((u8 *)(mem_re_addr + 2), (u8 *)mem_address, size);
			}
			else
			{
				memcpy((u8 *)(mem_re_addr + 2), (u8 *)mem_address, *((u32 *)mem_address-1));
			}
			mem_free_debug(mem_address);
			mem_address = NULL;
    	}
		if(mem_re_addr) 
		{
			*mem_re_addr = OS_MEM_FLAG;
			mem_re_addr ++;
			*mem_re_addr = length;
			mem_re_addr ++;
		}
    }
    else
    {
    	tls_os_sem_acquire(mem_sem, 0);
       // cpu_sr = tls_os_set_critical();
		mem_re_addr = (u32*)malloc(length);
		if(mem_re_addr && mem_address) 
		{
			if (*((u32 *)mem_address-1)> size)
			{
				memcpy((u8 *)(mem_re_addr + 2), (u8 *)mem_address, size);
			}
			else
			{
				memcpy((u8 *)(mem_re_addr + 2), (u8 *)mem_address, *((u32 *)mem_address-1));
			}
			*mem_re_addr = NON_OS_MEM_FLAG;
			mem_re_addr ++;
			*mem_re_addr = length;
			mem_re_addr ++;
			total_mem_size -= length;
		}
       // tls_os_release_critical(cpu_sr);	
		tls_os_sem_release(mem_sem);

		mem_free_debug(mem_address);
    }
#else
	tls_os_sem_acquire(mem_sem, 0);
	cpu_sr = tls_os_set_critical();
	mem_re_addr = realloc(mem_address, length);
	tls_os_release_critical(cpu_sr);
	tls_os_sem_release(mem_sem);
#endif	
	//if(mem_re_addr == NULL)
	//{
	//	printf("realloc error \r\n");
	//}
	return mem_re_addr;
}

void *mem_calloc_debug(u32 n, u32 size)
{
//    u32 cpu_sr = 0;
    u32 *buffer = NULL;
	u32 length = 0;

#if USING_ADD_HEADER
	length = n*size;
    length += 2*4;

    if(tls_get_isr_count() > 0)
    {
        extern void *pvPortMalloc( size_t xWantedSize );
        buffer = pvPortMalloc(length);
		if(buffer) 
		{
			memset(buffer, 0, length);
			*buffer = OS_MEM_FLAG;
			buffer ++;
			*buffer = length;
			buffer ++;
		}
    }
    else
    {
    	tls_os_sem_acquire(mem_sem, 0);
       // cpu_sr = tls_os_set_critical();
        buffer = (u32*)malloc(length);
		if(buffer) 
		{
			memset(buffer, 0, length);
			*buffer = NON_OS_MEM_FLAG;
			buffer ++;
			*buffer = length;
			buffer ++;
			total_mem_size -= length;
		}

      //  tls_os_release_critical(cpu_sr);	
		tls_os_sem_release(mem_sem);
    }
#else   //UCOSII
	tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    buffer = (u32*)calloc(n,size);
    tls_os_release_critical(cpu_sr); 
	tls_os_sem_release(mem_sem);
#endif
	//if(buffer == NULL)
	//{
	//	printf("calloc error \r\n");
	//}
	return buffer;
}
#endif /* WM_MEM_DEBUG */

extern u32 __heap_end;
extern u32 __heap_start;

u32 tls_mem_get_avail_heapsize(void)
{
#if USING_ADD_HEADER
	u32 availablemem = 0;
//	u32 cpu_sr;

	tls_os_sem_acquire(mem_sem, 0);
	//cpu_sr = tls_os_set_critical();
	availablemem = total_mem_size;
   // tls_os_release_critical(cpu_sr);	
	tls_os_sem_release(mem_sem);

	return availablemem&0xFFFFF000;
#else
	u8 *p = NULL;
	u32 startpos = 0;
	u32 stoppos = 0;
	u32 laststartpos = 0;
	static u32 last_avail_heapsize = 0;
	u32 cpu_sr = 0;

    if (!memory_manager_initialized) {
        tls_os_status_t os_status;
        memory_manager_initialized = true;
        //
        // NOTE: If two thread allocate the very first allocation simultaneously
        // it could cause double initialization of the memory manager. This is a
        // highly unlikely scenario and will occur in debug versions only.
        //
        os_status = tls_os_sem_create(&mem_sem, 1);
        if(os_status != TLS_OS_SUCCESS)
            printf("mem_alloc_debug: tls_os_sem_create mem_sem error\n");
    }

	tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();	
	if (last_avail_heapsize)
	{
		startpos = last_avail_heapsize;
		stoppos = last_avail_heapsize*2;
		if (startpos > ((u32)&__heap_end - (u32)&__heap_start))
		{
			startpos = (u32)&__heap_end - (u32)&__heap_start;
		}
	}
	else
	{
		startpos = (u32)&__heap_end - (u32)&__heap_start;
		stoppos = (u32)&__heap_end - (u32)&__heap_start;
	}

	for (;startpos <= stoppos;)
	{
		p = malloc(startpos);
		if (p)
		{
			free(p);
			if (startpos < 1024 || (stoppos - startpos) < 1024
				|| (startpos == ((u32)&__heap_end - (u32)&__heap_start)))
			{
				last_avail_heapsize = startpos;
				goto END;
			}
			laststartpos = startpos;
			startpos = (stoppos + startpos)>>1;
		}
		else
		{
			stoppos = startpos;
			if (laststartpos)
			{
				startpos = (laststartpos + stoppos)/2;
			}
			else
			{
				startpos = startpos>>1;
			}
			if (startpos < 1024 || (stoppos - startpos) < 1024)
			{
				last_avail_heapsize = startpos;
				goto END;
			}
		}
	}
END:
    tls_os_release_critical(cpu_sr); 
	tls_os_sem_release(mem_sem);	
	return startpos;
#endif	
}



