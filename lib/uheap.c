#include <inc/lib.h>
#include <inc/queue.h>

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code
	// panic("malloc() is not implemented yet...!!");
	// return NULL;
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy


	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		// cprintf("\n You should work! \n");
		if (sys_isUHeapPlacementStrategyFIRSTFIT()) {
			return alloc_block(size, DA_FF);
		} else if (sys_isUHeapPlacementStrategyBESTFIT()) {
			return alloc_block(size, DA_BF);
		} else {
			return NULL;
		}
	}

	// cprintf("\n You shouldn't work! \n");

	void* va = NULL;
	va = sys_search_user_mem(size);
	// cprintf("\n%~[1.1] internal %p \n", va);

	if (va == NULL) {
		return NULL;
	}

	// cprintf("\n%~[1.2] internal %p \n", va);

	sys_allocate_user_mem((uint32)va, size);


	// cprintf("\n%~[1.3] internal %p \n", va);

    return va; // No suitable slot for whatever reason.
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");

	uint32 va = (uint32) virtual_address;

	if(va < (uint32)myEnv->START || va > USER_HEAP_MAX)
	{
		panic("virtual_address is out of USER_HEAP boundries");
		return;
	}
	if(virtual_address < sbrk(0))
	{
		free_block(virtual_address);
		return;
	}

	if(va < (uint32)myEnv->HARD_LIMIT + PAGE_SIZE)
	{
		panic("virtual_address between BREAK and HARD_LIMIT");
		return;
	}

	sys_free_user_mem(va, 0);
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================

void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) {
		panic("\n size is 0 in smalloc \n");
		return NULL;
	}

	void* virtual_address = sys_search_user_mem(size);
	if (virtual_address == NULL) {
		panic("\n No user mem for smalloc! \n");
		return NULL;
	}
	int ret = sys_createSharedObject(sharedVarName, size, isWritable, virtual_address);
	if (ret == E_SHARED_MEM_EXISTS || ret == E_NO_SHARE) {
		panic("\n No kernel mem for smalloc (or shared exists, check ret value)! ret = %d \n", ret);
		return NULL;
	}
	uint32 page_num = ((uint32)(virtual_address) - USER_HEAP_START) >> 12;
	ids[page_num] = ret;
	//cprintf("\n ID: %d \n", ids[page_num]);
	return (void*) virtual_address;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================

void* sget(int32 ownerEnvID, char *sharedVarName) {
	uint32 size = sys_getSizeOfSharedObject(ownerEnvID, sharedVarName);
	void* virtual_address = sys_search_user_mem(size);
	if (virtual_address == NULL) {
		return NULL;
	}
	int ret = sys_getSharedObject(ownerEnvID, sharedVarName, virtual_address);
	if (ret == E_SHARED_MEM_EXISTS || ret == E_NO_SHARE) {
		return NULL;
	}
	uint32 page_num = ((uint32)(virtual_address) - USER_HEAP_START) >> 12;
	ids[page_num] = ret;
	//cprintf("\n ID: %d \n", ids[page_num]);
	return (void*) virtual_address;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	//panic("sfree() is not implemented yet...!!");

	uint32 page_num = ((uint32)(virtual_address) - USER_HEAP_START) >> 12;

	if (ids[page_num] == 0){ // Assume kheap allocation starts after INT_MAX + 1, that is 1000...(31 zeros)
		return;
	}
	//sys_get_shareID_with_va(virtual_address);
	//cprintf("\n ID: %d \n", id);
	sys_freeSharedObject(ids[page_num], virtual_address);
}

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
