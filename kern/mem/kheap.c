#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
#include <kern/conc/spinlock.h>
#define MAX_FRAMES (1 << 20)

unsigned int reverse_mapping[MAX_FRAMES] = { 0 };

//----------------------------------------------
uint32 first_free_address;
bool holes_created = 0;
struct spinlock plk;
uint32 FREE_PAGES_COUNT;

int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit) {
	if (initSizeToAllocate > (daLimit - daStart)) {
		panic("Kernel heap initialization failed: initial size exceeds limit.");
		return -1;
	}

	if (daStart % PAGE_SIZE != 0 || daLimit % PAGE_SIZE != 0 || (daStart + initSizeToAllocate) % PAGE_SIZE != 0) {
		panic("some address is invalid");
		return -1;
	}

	for (uint32 addr = daStart; addr < daStart + initSizeToAllocate; addr += PAGE_SIZE)
	{
		struct FrameInfo *frame_alloc = NULL;
		int allocation_success = allocate_frame(&frame_alloc);

		if (allocation_success == E_NO_MEM) {
			panic("kheap initialization frames allocation failed Insufficient memory\n");
			return -1;
		}

		int mapping_success = map_frame(ptr_page_directory, frame_alloc, addr, (PERM_PRESENT | PERM_USER | PERM_WRITEABLE));

		if (mapping_success == E_NO_MEM) {
			panic("kheap initialization frames mapping failed Insufficient memory\n");
			return -1;
		}
		reverse_mapping[to_physical_address(frame_alloc) >> 12] = addr;
	}

	BLOCK_ALLOCATION_START = (void*) daStart;
	BREAK = (void*) (daStart + initSizeToAllocate);
	HARD_LIMIT = (void*) daLimit;
	PAGE_ALLOCATION_START = (void*) ((int) HARD_LIMIT + PAGE_SIZE);
	first_free_address = (uint32)PAGE_ALLOCATION_START;
	init_spinlock(&plk, "lock for ptr for fast kheap allocation test");

	initialize_dynamic_allocator(daStart, initSizeToAllocate);
	return 0;
}

void* sbrk(int numOfPages) {
	if (numOfPages == 0) {
		return (void*) BREAK;
	}

	uint32 new_brk_pos = (uint32) BREAK + (numOfPages * PAGE_SIZE);

	if (new_brk_pos > (uint32) HARD_LIMIT) {
		return (void*) -1;
	}

	// allocating and mapping new pages
	for (uint32 addr = (uint32) BREAK; addr < new_brk_pos; addr += PAGE_SIZE)
	{
		struct FrameInfo *frame_alloc = NULL;
		int allocation_success = allocate_frame(&frame_alloc);

		if (allocation_success == E_NO_MEM) {
			panic("sbrk frames allocation failed Insufficient memory\n");
			return (void*) -1;
		}

		int mapping_success = map_frame(ptr_page_directory, frame_alloc, addr, (PERM_PRESENT | PERM_USER | PERM_WRITEABLE));

		if (mapping_success == E_NO_MEM) {
			panic("sbrk frames mapping failed Insufficient memory\n");
			return (void*) -1;
		}
		reverse_mapping[to_physical_address(frame_alloc) >> 12] = addr;
	}

	uint32 prev_brk = (uint32) BREAK;
	BREAK = (void*) new_brk_pos;
	return (void*) prev_brk;
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(unsigned int size) {
	if (!isKHeapPlacementStrategyFIRSTFIT()) {
		return NULL;
	}
	if (size <= PAGE_SIZE / 2) {
		return alloc_block_FF(size);
	}
	acquire_spinlock(&MemFrameLists.mfllock);
	if (MemFrameLists.free_frame_list.size < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE) {
		release_spinlock(&MemFrameLists.mfllock);
		return NULL;
	}
	acquire_spinlock(&plk);
	uint32 r = (holes_created ? (uint32)PAGE_ALLOCATION_START : (uint32) first_free_address);
	uint32 l = r;
	uint32 cnt = 0;
	while (r < KERNEL_HEAP_MAX) { // KHM is a constant ------- WARNING, untested change of comparison operator here.
		uint32* page_table;
		get_page_table(ptr_page_directory, r, &page_table); // Can't find a `create` parameter
		if (page_table[PTX(r)] & PERM_PRESENT) {
			r += PAGE_SIZE;
			l = r;
			cnt = 0;
		} else {
			cnt++;
			if (cnt * PAGE_SIZE >= size) {
				uint32 tl = l;
				while (l <= r) {
					struct FrameInfo* frame;
					allocate_frame(&frame);
					if (l == tl) {
						frame->size = cnt;
					}
					map_frame(ptr_page_directory, frame, l, PERM_WRITEABLE);
					reverse_mapping[to_physical_address(frame) >> 12] = l;
					l += PAGE_SIZE;
				}
				release_spinlock(&MemFrameLists.mfllock);
				release_spinlock(&plk);
				first_free_address = l;
				return (void*) tl;
			}
			r += PAGE_SIZE;
		}
	}
	release_spinlock(&plk);
	release_spinlock(&MemFrameLists.mfllock);
	return NULL;
}

void kfree(void* virtual_address) {
	acquire_spinlock(&MemFrameLists.mfllock);
	if (virtual_address < sbrk(0)) {
		release_spinlock(&MemFrameLists.mfllock);
		free_block(virtual_address);
	} else if ((uint32)virtual_address >= (uint32)HARD_LIMIT + PAGE_SIZE && (uint32)virtual_address <= KERNEL_HEAP_MAX) {
		uint32* ptr_page_table = NULL;
		get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_page_table);
		if (ptr_page_table == NULL || (ptr_page_table[PTX(virtual_address)] & PERM_PRESENT) == 0) {
			release_spinlock(&MemFrameLists.mfllock);
			return;
		}
		acquire_spinlock(&MemFrameLists.mfllock);
		// Didn't use plk here cuz the mfllock shall do the job.
		holes_created = 1;
		virtual_address = (void*) ROUNDDOWN((uint32 )virtual_address, PAGE_SIZE);
		struct FrameInfo *f = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
		uint32 n = f->size;
		f->size = -1;
		for (uint32 i = 0; i < n; i++) {
			unmap_frame(ptr_page_directory, (uint32)virtual_address);
			virtual_address += PAGE_SIZE;
		}
		release_spinlock(&MemFrameLists.mfllock);
	}
}
//void* slow_kmalloc(uint32 size) {
//	if (!isKHeapPlacementStrategyFIRSTFIT()) {
//		return NULL;
//	}
//	if (size <= PAGE_SIZE / 2) {
//		return alloc_block_FF(size);
//
//	}
//	acquire_spinlock(&MemFrameLists.mfllock);
//	if (MemFrameLists.free_frame_list.size < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE) {
//		release_spinlock(&MemFrameLists.mfllock);
//		return NULL;
//	}
//	uint32 r = (uint32) PAGE_ALLOCATION_START;
//	uint32 l = r;
//	uint32 cnt = 0;
//	while (r != KERNEL_HEAP_MAX) { // KHM is a constant
//		uint32* page_table;
//		get_page_table(ptr_page_directory, r, &page_table); // Can't find a `create` parameter
//		if (page_table[PTX(r)] & PERM_PRESENT) {
//			r += PAGE_SIZE;
//			l = r;
//			cnt = 0;
//		} else {
//			cnt++;
//			if (cnt * PAGE_SIZE >= size) {
//				uint32 tl = l;
//				while (l <= r) {
//					struct FrameInfo* frame;
//					allocate_frame(&frame);
//					map_frame(ptr_page_directory, frame, l, PERM_WRITEABLE);
//					l += PAGE_SIZE;
//				}
//				//get_page_table(ptr_page_directory, r, &page_table);
//				page_table[PTX(r)] |= 1 << 9; // 11 10 9 available bits // use 9th bit as "segment end" flag
//				release_spinlock(&MemFrameLists.mfllock);
//				return (void*) tl;
//			}
//			r += PAGE_SIZE;
//		}
//	}
//	release_spinlock(&MemFrameLists.mfllock);
//	return NULL; // No suitable slot for whatever reason.
////	uint32 ret = -1;
////		if(size <= DYN_ALLOC_MAX_BLOCK_SIZE){
////			return alloc_block_FF(size);
////		}
////		else{
////				 if(isKHeapPlacementStrategyFIRSTFIT()){
////				 uint32 pages_to_alloc = ROUNDUP( size , PAGE_SIZE) / PAGE_SIZE;
////				 uint32 counter = 0;
////				 uint32 va = (uint32)HARD_LIMIT + PAGE_SIZE;
////				 if (MemFrameLists.free_frame_list.size < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE) {
////				 		return NULL;
////				 	}
////
////				 while(va != KERNEL_HEAP_MAX){
////					 uint32* page_table_ret = NULL;
////					 uint32  start_va_to_map;
////					 struct FrameInfo* return_frame = get_frame_info(ptr_page_directory,va,&page_table_ret);
////					 if(return_frame == NULL){
////						 counter++;
////						 if(counter == 1) start_va_to_map = va;
////						 if(counter == pages_to_alloc){
////							 ret = start_va_to_map;
////							 for(int i = 0 ; i < counter ;i++){
////								struct FrameInfo * ptr = NULL;
////								int r = allocate_frame(&ptr);
////								map_frame(ptr_page_directory , ptr ,start_va_to_map, PERM_WRITEABLE);
////								if(i==0){
////									ptr->size = pages_to_alloc;
////								}else{
////									ptr->size = -1;
////								}
////
////								start_va_to_map += PAGE_SIZE;
////							 }
////							 return (void *)ret;
////						 }
////					 }
////					 else{
////						counter = 0;
////					 }
////
////					 va = va + PAGE_SIZE;
////				 }
////			 }
////		}
////		return NULL;
//}
//void slow_kfree(void* virtual_address) {
//	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
//	// Write your code here, remove the panic and write your code
//	//panic("kfree() is not implemented yet...!!");
//
//	//you need to get the size of the given allocation using its address
//	//refer to the project presentation and documentation for details
//
//	if ((uint32) virtual_address < KERNEL_HEAP_START || (uint32) virtual_address >= KERNEL_HEAP_MAX) {
//		panic("ERROR - INVALID ADDRESS | OUT OF KERNEL_HEAP");
//	} else if (virtual_address < sbrk(0)) {
//		free_block(virtual_address);
//	} else if (virtual_address >= HARD_LIMIT + PAGE_SIZE) {
//		uint32* ptr_page_table = NULL;
//		get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_page_table);
//		if (ptr_page_table == NULL) {
//			return;
//		}
//		if ((ptr_page_table[PTX(virtual_address)] & PERM_PRESENT) == 0) {
//			return;
//		}
//
//		virtual_address = (void*) ROUNDDOWN((uint32 )virtual_address, PAGE_SIZE);
//
//		while (virtual_address < (void*) KERNEL_HEAP_MAX) {
//
//			unmap_frame(ptr_page_directory, (uint32) virtual_address);
//
//			get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_page_table);
//
//			if (ptr_page_table[PTX(virtual_address)] & (1 << 9)) {
//				ptr_page_table[PTX(virtual_address)] &= ~(1 << 9);
//				break;
//			}
//
//			virtual_address = (virtual_address) + PAGE_SIZE;
//			if (virtual_address >= (void*) KERNEL_HEAP_MAX) {
//				break;
//			}
//		}
//	} else {
//		panic("ERROR - INVALID ADDRESS | BETWEEN sbrk AND HARD_LIMIT");
//	}
//
////	uint32 va = (uint32)virtual_address;
////		uint32* page_table_ret = NULL;
////	    struct FrameInfo* return_frame = get_frame_info(ptr_page_directory,va,&page_table_ret);
////
////	    if(return_frame!=NULL){
////
////	    	if(virtual_address >= (void *)KERNEL_HEAP_START&&
////	    	   virtual_address <=(void *) sbrk(0)){
////	    		free_block(virtual_address);
////
////			}
////			else if (virtual_address >=(void *) (HARD_LIMIT + PAGE_SIZE)&&
////					 virtual_address <=(void *) KERNEL_HEAP_MAX){
////
////				uint32 frames_to_free = return_frame->size;
////				return_frame->size = -1;
////				for(int i = 0;i<frames_to_free;i++){
////					unmap_frame(ptr_page_directory,va);
////					va+=PAGE_SIZE;
////				}
////			}
////			else{
////				panic("kfree() invalid virtual address !!");
////			}
////
////	    }
////	    else{
////	    	panic("kfree() invalid virtual address !!");
////	    }
//
//	/*
//	 Unmap
//	 - unmap_frame(ptr_page_directory, va)
//
//
//	 Freeframe
//	 - from va to pa
//	 - to_frame_into(pa)
//	 - free_frame(struct Frame_Info*)
//	 */
//}

unsigned int kheap_physical_address(unsigned int virtual_address) {
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	/*if(virtual_address >= KERNEL_HEAP_MAX){
	 return 0;
	 }
	 */
	uint32 *ptr_page_table = NULL;
	get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	if (ptr_page_table != NULL) {
		uint32 table_entry = ptr_page_table[PTX(virtual_address)];
		if ((table_entry & PERM_PRESENT) == 0)
			return 0;

		uint32 frame_num = table_entry >> 12;
		uint32 offset = virtual_address & ((1 << 12) - 1);
		uint32 physical_address = (frame_num * PAGE_SIZE) + offset;
		return physical_address;
	}
	return 0;
	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address) {
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	uint32 frame_num = physical_address / PAGE_SIZE;
	uint32 offset = physical_address % PAGE_SIZE;

	struct FrameInfo *frame_info = to_frame_info(physical_address);
	if (frame_info->references == 0 || reverse_mapping[frame_num] == 0)
		return 0;

	uint32 page_num = reverse_mapping[frame_num] >> 12;
	uint32 virtual_address = (page_num << 12) + offset;
	return virtual_address;
	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size) {
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code

	return NULL;
}
