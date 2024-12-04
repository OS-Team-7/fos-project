/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
#include "../inc/memlayout.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
uint32* start_block;
uint32* end_block;
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
    //==================================================================================
    //DON'T CHANGE THESE LINES==========================================================
    //==================================================================================
    {
        if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
        if (initSizeOfAllocatedSpace == 0)
            return ;
        is_initialized = 1;
    }
    //==================================================================================
    //==================================================================================

    //TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("initialize_dynamic_allocator is not implemented yet");
    //Your Code is Here...

    // Create start and end block and initialize their value with 0/1
    start_block = (uint32*) daStart;
    end_block = (uint32*) (daStart + initSizeOfAllocatedSpace - sizeof(uint32));


	cprintf("\nHere we go 101 %p\n", start_block);
	cprintf("\nHere we go 101 %p\n", end_block);

	cprintf("\nHere we go 101\n");
    *start_block = 1;
	cprintf("\nHere we go 102\n");
    *end_block = 1;
	cprintf("\nHere we go 103\n");

    // Create one free block with all available memory and add it to freeBlocksList
    uint32* free_block_header = (uint32*) ((uint32)start_block + sizeof(uint32));
    uint32* free_block_footer = (uint32*) ((uint32)end_block - sizeof(uint32));

    *free_block_header = (uint32)free_block_footer - (uint32)free_block_header + sizeof(uint32);
    *free_block_footer = *free_block_header;

    struct BlockElement* free_block_content = (struct BlockElement*) ((uint32)free_block_header + sizeof(uint32));

    LIST_INIT(&freeBlocksList);
    LIST_INSERT_HEAD(&freeBlocksList, free_block_content);
}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
    //TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("set_block_data is not implemented yet");
    //Your Code is Here...

    if (totalSize < 16 || totalSize % 2 != 0) {
        panic("set_block_data(): totalSize can't be less than 16 and must be even.");
    }

    uint32* block_header = (uint32*)va - 1;
    uint32* block_footer = (uint32*)((uint32)va + totalSize - 2 * sizeof(uint32));

    *block_header = totalSize + isAllocated;
    *block_footer = *block_header;
}


//=========================================
//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	if (size == 0) {
		return NULL;
	}

	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			// cprintf("\nDEBUG: User allocator getting initialized!\n");
			// initialize_kheap_dynamic_allocator(KERNEL_HEAP_START, PAGE_SIZE, KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE);
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			// cprintf("\nDEBUG: First sbrk\n");
			uint32 da_break = (uint32)sbrk(0);
			// cprintf("\nDEBUG: Second sbrk!\n");
			initialize_dynamic_allocator(da_start, da_break - da_start);
			// cprintf("\nDEBUG: init!\n");
		}
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_FF is not implemented yet");
	//Your Code is Here...

	struct BlockElement* fblock;
	uint32 free_block_size;
	// total = allocated_size + size of header and footer
	uint32 total_block_size = size + 2 * sizeof(uint32); // now the size is even and equal to at least 16

	LIST_FOREACH(fblock, &freeBlocksList) {
		free_block_size = get_block_size(fblock);
		// cprintf("DEBUG: size of free block: %d\n", free_block_size);
		// cprintf("DEBUG: size of total block size: %d\n", total_block_size);
		if (free_block_size >= total_block_size + 16) {
			// Create a new free block with unused memory space.
			void* new_free_block_va = (void*)((uint32)fblock + total_block_size);
			set_block_data(new_free_block_va, free_block_size - total_block_size, 0);
			LIST_INSERT_AFTER(&freeBlocksList, fblock, (struct BlockElement*)new_free_block_va);
			LIST_REMOVE(&freeBlocksList, fblock);

			set_block_data(fblock, total_block_size, 1);

			return fblock;
		} else if (free_block_size >= total_block_size) {
			LIST_REMOVE(&freeBlocksList, fblock);

			set_block_data(fblock, free_block_size, 1);

			return fblock;
		}
	}

	int prev_break = (int)sbrk(ROUNDUP(total_block_size, PAGE_SIZE)/PAGE_SIZE);

	if (prev_break == -1) {
	    return NULL;
	} else {
		uint32 new_break = (uint32)sbrk(0);
		uint32* new_end_block_pos = (uint32*)(new_break - sizeof(uint32));
		*new_end_block_pos = 1;
		set_block_data((void*)prev_break, (new_break - (uint32)prev_break), 1);
		free_block((void*)prev_break);
        return alloc_block_FF(size);
	}
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	if (size == 0) {
			return NULL;
		}

	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_BF is not implemented yet");
	//Your Code is Here...

	if (size == 0) {
		return NULL;
	}

	struct BlockElement* best_free_block = NULL;
	uint32 bf_block_size = 0;
	struct BlockElement* free_block;
	uint32 free_block_size;
	// total = allocated_size + size of header and footer
	uint32 total_block_size = size + 2 * sizeof(uint32); // now the size is even and equal to at least 16

	LIST_FOREACH(free_block, &freeBlocksList) {
		free_block_size = get_block_size(free_block);

		if (free_block_size >= total_block_size) {
			if (best_free_block == NULL || free_block_size < bf_block_size) {
				best_free_block = free_block;
				bf_block_size = free_block_size;
			}
		}
	}

	if (best_free_block != NULL) {
		if (bf_block_size >= total_block_size + 16) {
			// Create a new free block with unused memory space.
			void* new_free_block_va = (void*)((uint32)best_free_block + total_block_size);
			set_block_data(new_free_block_va, bf_block_size - total_block_size, 0);
			LIST_INSERT_AFTER(&freeBlocksList, best_free_block, (struct BlockElement*)new_free_block_va);
			LIST_REMOVE(&freeBlocksList, best_free_block);

			set_block_data(best_free_block, total_block_size, 1);

			return best_free_block;
		} else if (bf_block_size >= total_block_size) {
			LIST_REMOVE(&freeBlocksList, best_free_block);

			set_block_data(best_free_block, bf_block_size, 1);

			return best_free_block;
		}
	}

	if ((int)sbrk(ROUNDUP(total_block_size, PAGE_SIZE)/PAGE_SIZE) == -1) {
		return NULL;
	}
	else
		return alloc_block_BF(size);
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
    //TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("free_block is not implemented yet");
    //Your Code is Here...


    //cprintf("\n===\nBEFORE FREE\n===\n");
    //print_blocks_list(freeBlocksList);

    if (va == NULL)
        return;

    if(is_free_block(va))
    	return;

    char neighbor_is_free = 0;
    struct BlockElement *curr_block = va;
    uint32 prev_block_size = get_block_size((uint32*)curr_block - 1);
    uint32 curr_block_size = get_block_size(curr_block);
	uint32 next_block_size;

    struct BlockElement *prev_block = (void*)((uint32)curr_block - prev_block_size);
    struct BlockElement *next_block = (void*)((uint32)curr_block + curr_block_size);

    if (is_free_block(prev_block))
    {
    	neighbor_is_free = 1;

        set_block_data(prev_block, prev_block_size + curr_block_size, 0);

        curr_block = prev_block;
    	curr_block_size = get_block_size(curr_block);
    }
    if (is_free_block(next_block))
    {
      	if(neighbor_is_free > 0)
      	{
            LIST_REMOVE(&freeBlocksList, curr_block);
            set_block_data(curr_block, curr_block_size, 1);
      	}
      	else
    	  	neighbor_is_free = 1;

    	next_block_size = get_block_size(next_block);

        // flag @ header cur footer @ header free footer @ header allocated footer
    	// @ header free footer @flag
    	if(LIST_FIRST(&freeBlocksList) == next_block)
    		LIST_INSERT_HEAD(&freeBlocksList, curr_block);

    	// flag @ header free footer @ header allocated footer  @ header cur footer
    	// @ header free footer @ flag
      	else if(LIST_LAST(&freeBlocksList) == next_block)
            LIST_INSERT_TAIL(&freeBlocksList, curr_block);

    	// flag @ header free footer @ header allocated footer
    	// @ header cur footer @ header free footer @ header allocated footer
    	// @ header free footer @ flag
      	else
        	LIST_INSERT_AFTER(&freeBlocksList, next_block, curr_block);

      	LIST_REMOVE(&freeBlocksList, next_block);

        // header prev_of_next footer @ header allocated footer @ header curr footer @ header next footer

        set_block_data(curr_block, next_block_size + curr_block_size, 0);
    }

    // if there is no previous or next free block
    if (neighbor_is_free == 1)
    {
    	//cprintf("\n===\nAfter FREE\n===\n");
    	//print_blocks_list(freeBlocksList);
    	return;
    }


    // header free footer @ header allocated footer @ header cur footer @ header allocated footer @ header free footer

    void * iterator = curr_block; // footer of previous block

    // flag @ header content footer @ header cur footer

    while(!is_free_block(iterator) && get_block_size((uint32*)iterator - 1) > 0) // not the flag
    {
    	//cprintf("\n===\nDEBUG: Start Block %p!\n===\n", start_block);
    	//cprintf("\n===\nDEBUG: Iterator %p!\n===\n", iterator);
    	//cprintf("\n===\nDEBUG: Block size %d!\n===\n", get_block_size(iterator - 1));
    	prev_block_size = get_block_size((uint32*)iterator - 1);

    	iterator = (void*)((uint32)iterator - prev_block_size);
    }

    if(is_free_block(iterator))
    	LIST_INSERT_AFTER(&freeBlocksList, (struct BlockElement*)iterator, curr_block);
    else // previous was the flag
    	LIST_INSERT_HEAD(&freeBlocksList, curr_block);

    set_block_data(curr_block, curr_block_size, 0);


    //cprintf("\n===\nAfter FREE\n===\n");
    //print_blocks_list(freeBlocksList);
}



//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("realloc_block_FF is not implemented yet");

	if(va == NULL)
		return alloc_block_FF(new_size);

	if(is_free_block(va)) // block is not allocated to reallocate it
		return NULL;

	if(new_size < 1)
	{
		free_block(va);
		return NULL;
	}

	uint32 old_size = get_block_size(va);
	uint32 new_required_size = new_size + 8; // header & footer

	if(new_required_size < 16)
		new_required_size = 16;
	else if(new_required_size & 1)
		new_required_size++;

	if(new_required_size == old_size)
		return va;

	struct BlockElement *next_block = (struct BlockElement*)((char*)va + old_size);
	// ***** SHRINK *****
	if(new_required_size < old_size)
	{
		// next is free block ?
		if(is_free_block(next_block))
		{
			set_block_data(va, new_required_size, 1);

			// update the free block size
			// change pointers of previous/next free blocks

			struct BlockElement *next_block_prev = LIST_PREV(next_block);
			struct BlockElement *next_block_next = LIST_NEXT(next_block);

			set_block_data((char*) va + new_required_size,
					get_block_size(next_block) + (old_size - new_required_size), 0);

			next_block = (struct BlockElement *)((char*)va + new_required_size);

			next_block->prev_next_info.le_next = next_block_next;
			next_block->prev_next_info.le_prev = next_block_prev;

			if(LIST_FIRST(&freeBlocksList) != next_block) // flag |  | flag
				next_block_prev->prev_next_info.le_next = next_block;

			if(LIST_LAST(&freeBlocksList) != next_block)
				next_block_next->prev_next_info.le_prev = next_block;
		}
		else
		{
			if(old_size - new_required_size >= 16) // can make free block
			{
				set_block_data(va, new_required_size, 1);

				char* free = (char*) va + new_required_size;

				set_block_data(free, old_size - new_required_size, 1);

				free_block(free);
			}
			else
				return NULL; // do nothing // TA:RAZAN
		}
		return va; // return the va even if the shrink failed // DR: AHMAD SALAH
	}

	// ***** EXTEND *****

	// can add next block ? (extend to right)
	uint32 right_neighbor_size = get_block_size((char*)va + old_size);
	if(is_free_block(next_block) && right_neighbor_size + old_size >= new_required_size)
	{
		if(right_neighbor_size - (new_required_size - old_size) < 16) // merge the neighbor with cur
		{
			LIST_REMOVE(&freeBlocksList, next_block);

			set_block_data(va, old_size + right_neighbor_size, 1);
		}
		else // take a part from the neighbor
		{
			struct BlockElement *next_block_prev = LIST_PREV(next_block);
			struct BlockElement *next_block_next = LIST_NEXT(next_block);

			set_block_data(va, new_required_size, 1);

			set_block_data((char*)va + new_required_size,
					right_neighbor_size - (new_required_size - old_size), 0);

			next_block = (struct BlockElement *)((char*)va + new_required_size);

			next_block->prev_next_info.le_next = next_block_next;
			next_block->prev_next_info.le_prev = next_block_prev;

			if(LIST_FIRST(&freeBlocksList) != next_block) // flag |  | flag
				next_block_prev->prev_next_info.le_next = next_block;

			if(LIST_LAST(&freeBlocksList) != next_block)
				next_block_next->prev_next_info.le_prev = next_block;
		}
		return va;
	}

	// can/(need to) add previous block ? (extend to left) // TA: RAZAN & DR: AHMAD SALAH TOLD ME TO IGNORE THIS

	// ***** REALLOCATE *****
	char* new_va = alloc_block_FF(new_size);
	if(new_va == NULL) // can't allocate
	{
		return NULL; // failed to expand the size // sbrk is called at alloc_block_FF
		// should free before return when the reallocation fails? => NO // DR: AHMAD SALAH
	}

	// copy the data in case we allocated new block
	// header | 					data 						|footer									|
	//        |va[0]    		:			  va[(size - 1) - 8]|va[(size - 1) - 7] : va[(size - 1) - 4]|
	char* va_char = (char*) va;
	for(uint32 i = 0; i < old_size - 8; i++)
		new_va[i] = va_char[i];


	free_block(va);

	return new_va;
}



/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
