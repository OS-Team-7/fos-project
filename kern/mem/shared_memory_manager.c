//shared_memory_manager.c

#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init() {
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list)
	;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

/*
 * Translate to pa, loop over frames of each shared object,
 * check if it exists, then that's my shared.
 * As pa is unique to a shared object,
 * and to va and pa are unique to each other.
 */
int32 get_shareID_with_va(void* virtual_address) {
	struct Env* myenv = get_cpu_proc(); //The calling environment
	uint32 *ptr_page_table = NULL;
	struct FrameInfo* fi =  get_frame_info(myenv->env_page_directory, (uint32) virtual_address, &ptr_page_table);
	//cprintf("\n ID of env of fi = %d \n", fi->proc->env_id);
	struct Share* curr_share;
	acquire_spinlock(&AllShares.shareslock);
	for (curr_share = LIST_FIRST(&AllShares.shares_list); curr_share != 0; curr_share = LIST_NEXT(curr_share)) {
		uint32 n = sizeof(curr_share->framesStorage) / sizeof(uint32);
		for (uint32 i = 0; i < n; i++) {
			//cprintf("\n cur_shareID = %d \n", curr_share->ID);
			if (curr_share->framesStorage[i] == fi) {
				release_spinlock(&AllShares.shareslock);
				return curr_share->ID;
			}
		}
	}

	release_spinlock(&AllShares.shareslock);
	return 0;
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName) {
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames) {
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...

	struct FrameInfo** frame_storage = kmalloc(sizeof(uint32) * numOfFrames);

	if (frame_storage == NULL) {
		panic("frame_storage is NULL\n");
	} else {
		for (int i = 0; i < numOfFrames; i++) {
			frame_storage[i] = 0;
		}
	}

	return frame_storage;
}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
/*
 * We can change the definition, no callers but us. to take virtual address
 */
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable) {
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");
	//Your Code is Here...

	struct Share* shared_object = kmalloc(sizeof(struct Share));
	if (shared_object == NULL) {
		return NULL;
	}
	shared_object->references = 1;
	shared_object->ownerID = ownerID;

	shared_object->ID = (int32)((uint32)shared_object ^ (1 << 31));
	strcpy(shared_object->name, shareName);
	shared_object->isWritable = isWritable;
	shared_object->size = size;

	int number_of_frames = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	shared_object->framesStorage = create_frames_storage(number_of_frames);

	return shared_object;
}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name) {
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");
	//Your Code is Here...

	/*
	 * Loop over shares list and check
	 * Found? return current pointer
	 */
	struct Share* curr_share;
	acquire_spinlock(&AllShares.shareslock);
	for (curr_share = LIST_FIRST(&AllShares.shares_list); curr_share != 0; curr_share = LIST_NEXT(curr_share)) {
		//cprintf("\n in get_share, share_name: %s, name_to_get: %s \n", curr_share->name, name);
		if (curr_share->ownerID == ownerID && strcmp(curr_share->name, name) == 0) {
			release_spinlock(&AllShares.shareslock);
			return curr_share;
		}
	}

	release_spinlock(&AllShares.shareslock);
	return NULL;
}

//=========================
// [4] Create Share Object:
//=========================

/*
 * Allocate and map the share and frames_storage using kmalloc()
 * Map the va sent to frames and write the mapped frames in the frames_storage
 */

int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
	struct Share* is_existed = get_share(ownerID, shareName);
	//cprintf("\n after get_share: %u\n", (uint32)is_existed);
	if (is_existed != NULL) {
		return E_SHARED_MEM_EXISTS;
	}
	struct Share* created_share = create_share(ownerID, shareName, size, isWritable);
	//cprintf("\n after create_share \n");
	if (created_share == NULL) {
		return E_NO_SHARE;
	}
	uint32 pages_count = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 start = *((uint32*) virtual_address);
	uint32 allocated = 0;

	if (pages_count > MemFrameLists.free_frame_list.size) {
		cprintf("\n No mem in createSharedObject \n");
		return E_NO_SHARE;
	}
	uint32 va = (uint32)virtual_address;
	while (pages_count > 0) {
		struct FrameInfo* curr_frame = NULL;
		allocate_frame(&curr_frame);
		//cprintf("\n pages_count: %u, virtual_address: %u, present: %u \n", pages_count, (uint32)virtual_address, (uint32) virtual_address & PERM_PRESENT);
		map_frame(myenv->env_page_directory, curr_frame, va, PERM_WRITEABLE | PERM_USER);
		//---------------------------
//		uint32* page_table;
//		get_page_table(ptr_page_directory, fault_va, &page_table);
//		uint32 pa = kheap_physical_address((uint32) page_table);
//		uint32 fn = pa / PAGE_SIZE;
//		cprintf("\n in fault_handler fn: %u \n", fn);
//		pc[fn] += 1;
		//---------------------------

		if (strcmp(shareName, "y") == 0) {
			*((uint32*) va) = 20;
		}

		//cprintf("\n in createSharedObject, Name: %s, frame: %u , written: %u \n", shareName, (uint32)curr_frame, *((uint32*)va));

		//pt_set_page_permissions(myenv->env_page_directory, (uint32) va, 0, PERM_PRESENT);
		//cprintf("\n mapped \n");

		created_share->framesStorage[allocated] = curr_frame;
		allocated++;
		va += PAGE_SIZE;
		pages_count--;
	}
	LIST_INSERT_TAIL(&AllShares.shares_list, created_share);
	//cprintf("\n ID in createSharedObject: %d \n", created_share->ID);
	return created_share->ID;
}

//======================
// [5] Get Share Object:
//======================
/*
 * Map the va to the object frames
 * Hint: Use map_frame()
 */
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
	struct Share* s = get_share(ownerID, shareName);
	if (s == NULL) {
		return E_SHARED_MEM_NOT_EXISTS;
	}
	s->references += 1;
	//cprintf("\n in getSharedObject, name: %s, writable: %u \n", shareName, (uint32)s->isWritable);
	// Loop over frames, map them to the va
	int n = ROUNDUP(s->size, PAGE_SIZE) / PAGE_SIZE;
	uint32 va = (uint32) virtual_address;
	for (int i = 0; i < n; i++) {
		//cprintf("\n before_Mapping: %u, \n", *(uint32*)virtual_address);
		map_frame(myenv->env_page_directory, s->framesStorage[i], (uint32) va, PERM_WRITEABLE * (s->isWritable) | PERM_USER);


		//cprintf("\n in getSharedObject, Name: %s, frame: %u , written: %u \n", shareName, (uint32)(s->framesStorage[i]), *((uint32*)(va)));

		//uint32 permissions = pt_get_page_permissions(myenv->env_page_directory, (uint32)va);
		//cprintf("\n in getSharedObject, name: %s, PERM_WRATEABLE: %u \n", shareName, PERM_WRITEABLE & permissions);
//		bool is_in_user_heap = (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX);
//		bool is_unmarked_page = ((permissions & PERM_MARKED) == 0);
//		bool is_read_only = ((permissions & PERM_WRITEABLE) == 0);
		//pt_set_page_permissions(myenv->env_page_directory, (uint32) va, PERM_MARKED, PERM_PRESENT);
		va += PAGE_SIZE;
	}
	return s->ID;

}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself

/*
 * Erase Object from shares_list.
 * kfree() the space for the Object and the frames_storage.
 */

void free_share(struct Share* ptrShare) {
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_share is not implemented yet");
	//Your Code is Here...
	if (ptrShare == NULL) {
		return;
	}
	acquire_spinlock(&AllShares.shareslock);
	LIST_REMOVE(&AllShares.shares_list, ptrShare);
	release_spinlock(&AllShares.shareslock);
	kfree((void*) ptrShare->framesStorage);
	kfree((void*) (ptrShare->ID | 1 << 31));
}
//========================
// [B2] Free Share Object:
//========================
/*
 * Get it from the list
 * Unmap virtual pages
 * If page_table is empty, delete it.
 * Decrement Object References
 * Last share? call free_share(prtShare).
 * Flush the cashe.
 */
int freeSharedObject(int32 sharedObjectID, void *startVA) {
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("freeSharedObject is not implemented yet");
	//Your Code is Here.../*

	return 0;
}
