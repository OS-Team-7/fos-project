/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/proc/user_environment.h>
#include "kheap.h"
#include "memory_manager.h"
#include <inc/queue.h>
#include <inc/dynamic_allocator.h>

//extern void inctst();

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va on the given page_directory
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, cut-paste the number of pages and return 0
//	ALL 12 permission bits of the destination should be TYPICAL to those of the source
//	The given addresses may be not aligned on 4 KB
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	//[PROJECT] [CHUNK OPERATIONS] cut_paste_pages
	// Write your code here, remove the panic and write your code
	panic("cut_paste_pages() is not implemented yet...!!");
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	If ANY of the destination pages exists with READ ONLY permission, deny the entire process and return -1.
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages doesn't exist, create it with the following permissions then copy.
//	Otherwise, just copy!
//		1. WRITABLE permission
//		2. USER/SUPERVISOR permission must be SAME as the one of the source
//	The given range(s) may be not aligned on 4 KB
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
	//[PROJECT] [CHUNK OPERATIONS] copy_paste_chunk
	// Write your code here, remove the //panic and write your code
	panic("copy_paste_chunk() is not implemented yet...!!");
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	It should set the permissions of the second range by the given perms
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, share the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	The given range(s) may be not aligned on 4 KB
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	//[PROJECT] [CHUNK OPERATIONS] share_chunk
	// Write your code here, remove the //panic and write your code
	panic("share_chunk() is not implemented yet...!!");
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate the given virtual range [<va>, <va> + <size>) in the given address space  <page_directory> with the given permissions <perms>.
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, allocate the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	Allocation should be aligned on page boundary. However, the given range may be not aligned.
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	//[PROJECT] [CHUNK OPERATIONS] allocate_chunk
	// Write your code here, remove the //panic and write your code
	panic("allocate_chunk() is not implemented yet...!!");
}

//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{
	//[PROJECT] [CHUNK OPERATIONS] calculate_allocated_space
	// Write your code here, remove the panic and write your code
	panic("calculate_allocated_space() is not implemented yet...!!");
}

//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
//This function should calculate the required number of pages for allocating and mapping the given range [start va, start va + size) (either for the pages themselves or for the page tables required for mapping)
//	Pages and/or page tables that are already exist in the range SHOULD NOT be counted.
//	The given range(s) may be not aligned on 4 KB
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{
	//[PROJECT] [CHUNK OPERATIONS] calculate_required_frames
	// Write your code here, remove the panic and write your code
	panic("calculate_required_frames() is not implemented yet...!!");
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
/* DYNAMIC ALLOCATOR SYSTEM CALLS */
//=====================================
/*
void initialize_user_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	// cprintf("\n    Start of User Dynamic Allocator Initialization   \n");
	struct Env* env = get_cpu_proc(); //the current running Environment to adjust its break limit

    //==================================================================================
    //DON'T CHANGE THESE LINES==========================================================
    //==================================================================================
    {
        if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
        if (initSizeOfAllocatedSpace == 0)
            return ;
        env->is_initialized = 1;
    }
    //==================================================================================
    //==================================================================================

    //TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("initialize_dynamic_allocator is not implemented yet");
    //Your Code is Here...

    // Create start and end block and initialize their value with 0/1
    env->start_block = (uint32*) daStart;
    env->end_block = (uint32*) (daStart + initSizeOfAllocatedSpace - sizeof(uint32));


	//cprintf("\nHere we go 101 %p\n", env->end_block);

	//cprintf("\nHere we go 101\n");
    *env->start_block = 1;
	//cprintf("\nHere we go 102\n");
    *env->end_block = 1;
	//cprintf("\nHere we go 103\n");

    // Create one free block with all available memory and add it to freeBlocksList
    uint32* free_block_header = (uint32*) ((uint32)env->start_block + sizeof(uint32));
    uint32* free_block_footer = (uint32*) ((uint32)env->end_block - sizeof(uint32));

	//cprintf("\nHere we go 104\n");

    *free_block_header = (uint32)free_block_footer - (uint32)free_block_header + sizeof(uint32);
    *free_block_footer = *free_block_header;

	//cprintf("\nHere we go 105\n");

    struct BlockElement* free_block_content = (struct BlockElement*) ((uint32)free_block_header + sizeof(uint32));

	//cprintf("\nHere we go 106\n");
    LIST_INIT(&(env->freeBlocksList));
	//cprintf("\nHere we go 107\n");
    LIST_INSERT_HEAD(&(env->freeBlocksList), free_block_content);
	//cprintf("\n    End of User Dynamic Allocator Initialization   \n");
}
*/
void* sys_sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the current user program to increase the size of its heap
	 * 				by the given number of pages. You should allocate NOTHING,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) As in real OS, allocate pages lazily. While sbrk moves the segment break, pages are not allocated
	 * 		until the user program actually tries to access data in its heap (i.e. will be allocated via the fault handler).
	 * 	2) Allocating additional pages for a process� heap will fail if, for example, the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sys_sbrk fails, the net effect should
	 * 		be that sys_sbrk returns (void*) -1 and that the segment break and the process heap are unaffected.
	 * 		You might have to undo any operations you have done so far in this case.
	 */

	//TODO: [PROJECT'24.MS2 - #11] [3] USER HEAP - sys_sbrk
	/*====================================*/
	/*Remove this line before start coding*/
	//return (void*)-1 ;
	/*====================================*/

	struct Env* env = get_cpu_proc(); //the current running Environment to adjust its break limit


	// cprintf("\nHere we go env_id %d\n", env->env_id);
	// cprintf("\nHere we go %p, %p\n", env->START, env->BREAK);
	// cprintf("\nHere we go pages %d\n", numOfPages);
	uint32 prev_break = (uint32)env->BREAK;

	if (numOfPages == 0)
	{
		return (void*) prev_break;
	}

	uint32 new_break = (uint32)((uint32)(env->BREAK) + (numOfPages * PAGE_SIZE));

	if (new_break > (uint32)env->HARD_LIMIT)
	{
		return (void*) -1;
	}

	// cprintf("\n%~[1.7.2] Variables prev_break %p new_break %p diff %d \n", prev_break, new_break, new_break - prev_break);

	for (uint32 curr_page = prev_break; curr_page < new_break; curr_page += PAGE_SIZE)
	{
    	// cprintf("\n%~[1.7.2] Page is getting marked %p \n", curr_page);
    	uint32* page_table;

        if (get_page_table(env->env_page_directory, curr_page, &page_table) == TABLE_NOT_EXIST) {
            page_table = create_page_table(env->env_page_directory, curr_page);
        	// cprintf("\n%~[1.7.2] Page table got created \n");
        }
		pt_set_page_permissions(env->env_page_directory, curr_page, PERM_USER | PERM_WRITEABLE | PERM_MARKED, 0);

    	// cprintf("\n%~[1.7.2] Page got marked %p \n", curr_page);
	}

	env->BREAK = (void*)new_break;

	// cprintf("\nStart is here\n");

	// cprintf("\nHere we go 2\n");

	return (void*) prev_break;
}


//=====================================
// 0) SEARCH USER MEM: Find a va that has the free size.
//=====================================
void* search_user_mem(struct Env* e, uint32 size) {
	// cprintf("\n%~[1.4] internal hard limit %p \n", e->HARD_LIMIT);

	/*
	 * **IMP** can change the next line
	 */
	//cprintf("\n e->holes_created: %d \n", e->holes_created);
	acquire_spinlock(&umlk);
    uint32 r = (e->holes_created ? ((uint32)e->HARD_LIMIT + PAGE_SIZE) : e->first_free_address);
    uint32 l = r;
    uint32 cnt = 0;
    while(r + PAGE_SIZE - 1 < USER_HEAP_MAX) {
        uint32* page_table;
        if (get_page_table(e->env_page_directory, r, &page_table) == TABLE_NOT_EXIST) {
            page_table = create_page_table(e->env_page_directory, r);
        }
        if (page_table[PTX(r)] & (PERM_MARKED | PERM_PRESENT)) {
        	// cprintf("\n%~[1.6] internal PERM_MARKED %p \n", r);
            r += PAGE_SIZE;
            l = r;
            cnt = 0;
        } else {
            cnt++;
            if (cnt * PAGE_SIZE >= size) {
            	e->first_free_address = l + cnt * PAGE_SIZE;
            	release_spinlock(&umlk);
            	return (void*)l;
            }
            r += PAGE_SIZE;
        }
    }
    release_spinlock(&umlk);
    return NULL;
}

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	// cprintf("\n%~[1.3.2] internal \n");
    /*====================================*/
    /*Remove this line before start coding*/
//    inctst();
//    return;
    /*====================================*/

    //TODO: [PROJECT'24.MS2 - #13] [3] USER HEAP [KERNEL SIDE] - allocate_user_mem()
    // Write your code here, remove the panic and write your code
    //panic("allocate_user_mem() is not implemented yet...!!");

    uint32* ptr_page_table = NULL;

    virtual_address = ROUNDDOWN(virtual_address, PAGE_SIZE);

    uint32 pages_count = size / PAGE_SIZE + (size % PAGE_SIZE != 0);

    for(uint32 i = 0; i < pages_count; i++)
    {
    	if (get_page_table(e->env_page_directory, virtual_address + i * PAGE_SIZE, &ptr_page_table) == TABLE_NOT_EXIST){
    		create_page_table(e->env_page_directory, virtual_address + i * PAGE_SIZE);
    	}

        pt_set_page_permissions(e->env_page_directory, virtual_address + i * PAGE_SIZE, PERM_MARKED | PERM_WRITEABLE | PERM_USER, 0);
    }

    pt_set_page_permissions(e->env_page_directory, virtual_address + (pages_count - 1) * PAGE_SIZE, (1 << 9), 0);


    /*for(uint32 va = virtual_address ;va < virtual_address + size; va += PAGE_SIZE){
        if (get_page_table(e->env_page_directory, va, &ptr_page_table) == TABLE_NOT_EXIST){
            create_page_table(e->env_page_directory, va);
        }

    	// cprintf("\n%~[1.7.1] Page got marked %p \n", va);
        pt_set_page_permissions(e->env_page_directory, va, PERM_MARKED | PERM_WRITEABLE | PERM_USER, 0);
    }
    pt_set_page_permissions(e->env_page_directory, ROUNDDOWN(virtual_address + size - PAGE_SIZE, PAGE_SIZE), (1 << 9), 0);
*/
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	/*====================================*/
	/*Remove this line before start coding*/
//	inctst();
//	return;
	/*====================================*/

	//TODO: [PROJECT'24.MS2 - #15] [3] USER HEAP [KERNEL SIDE] - free_user_mem
	// Write your code here, remove the panic and write your code
	//panic("free_user_mem() is not implemented yet...!!");

	e->holes_created = 1;
	virtual_address = ROUNDDOWN(virtual_address, PAGE_SIZE);
	uint32* ptr_page_table = NULL;
	while(1)
	{
		if(get_page_table(e->env_page_directory, virtual_address,&ptr_page_table) == TABLE_NOT_EXIST)
			break;

		if((ptr_page_table[PTX(virtual_address)] & PERM_MARKED) < 1)
			break;


		if(ptr_page_table[PTX(virtual_address)] & PERM_PRESENT)
			env_page_ws_invalidate(e, virtual_address);

		pf_remove_env_page(e, virtual_address);

		ptr_page_table[PTX(virtual_address)] &= ~(PERM_MARKED);
		if(ptr_page_table[PTX(virtual_address)] & (1 << 9)) // end of segment
		{
			ptr_page_table[PTX(virtual_address)] &= ~(1 << 9);
			break;
		}

		virtual_address += PAGE_SIZE;
	}

	//TODO: [PROJECT'24.MS2 - BONUS#3] [3] USER HEAP [KERNEL SIDE] - O(1) free_user_mem
}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//[PROJECT] [USER HEAP - KERNEL SIDE] move_user_mem
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//

