#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
#define MAX_FRAMES (1 << 20)
unsigned int reverse_mapping[MAX_FRAMES] = {0};
int8 enable_fast_page_allocation = 1;

//--------------------------------- SegTree code

Node merge(Node parent, Node left, Node right) {
	if (left.size >= right.size) {
		parent.size = left.size;
		parent.address = left.address;
	} else {
		parent.size = right.size;
		parent.address = right.address;
	}
	return parent;
}

uint32 seg_tree_initialized = 0;
//#define UINT_MAX 4294967295
void add(uint32 address, uint32 size) {
	add1(1, 0, (uint32) KERNEL_HEAP_MAX - 1, address, size);
}

void add1(uint32 ti, uint32 tl, uint32 tr, uint32 address, uint32 size) {
	//cprintf("address: %u, ti: %u, tl: %u, tr: %u\n",address, ti, tl, tr);
//	if (size == 2 * 8 * 1024) {
//		cprintf("address: %u, ti: %u, tl: %u, tr: %u\n", address, ti, tl, tr);
//	}
//	if (ti == 524318) {
//		cprintf("in_add: l_size, %u r_size %u\n", slot[slot[ti].l].size, slot[slot[ti].r].size);
//	}
	if (tl == tr) {
		if (tl != address) {
			panic("tl != address");
		}
		slot[ti].size = size;
		//if (ti == 524318) {
		//	cprintf("in_add2: ti %u, size %u, l_size, %u r_size %u\n",ti,size, slot[slot[ti].l].size, slot[slot[ti].r].size);
		//}
		slot[ti].address = address;
		//panic("ahh");
		slot[ti].l = 0;
		slot[ti].r = 0;
		return;
	}
	const uint32 mid = ((unsigned long long) tl + tr) / 2;
	if (mid < address) {
		if (slot[ti].r == 0) {
			slot[ti].r = node_counter++;
		}
		add1(slot[ti].r, mid + 1, tr, address, size);
	} else {
		if (slot[ti].l == 0) {
			slot[ti].l = node_counter++;
		}
		add1(slot[ti].l, tl, mid, address, size);
	}
	//cprintf("ti: %u, l: %u, r: %u\n", ti, slot[ti].l, slot[ti].r);
	if (slot[ti].l == 0) {
		slot[ti].address = slot[slot[ti].r].address;
		slot[ti].size = slot[slot[ti].r].size;
	} else if (slot[ti].r == 0) {
		slot[ti].address = slot[slot[ti].l].address;
		slot[ti].size = slot[slot[ti].l].size;
	} else {
		slot[ti] = merge(slot[ti], slot[slot[ti].l], slot[slot[ti].r]);
	}
	if ((slot[ti].l && slot[slot[ti].l].size > slot[ti].size) || (slot[ti].r && slot[slot[ti].r].size > slot[ti].size)) {
		panic("\nti %u l %u r %u size %u l_size %u r_size %u\n", ti, slot[ti].l, slot[ti].r, slot[ti].size, slot[slot[ti].l].size, slot[slot[ti].r].size);
	}
}

// Check if address exists
Node get(uint32 address) {
	return get1(1, 0, (uint32) KERNEL_HEAP_MAX - 1, address);
}

Node get1(uint32 ti, uint32 tl, uint32 tr, uint32 address) {
	if (slot[ti].address == address) {
		return slot[ti];
	}
	uint32 mid = ((unsigned long long) tl + tr) / 2;
	if (mid < address) {
		if (slot[ti].r == 0) {
			Node ret;
			ret.address = 0;
			ret.size = 0;
			ret.l = 0;
			ret.r = 0;
			return ret;
		}
		return get1(slot[ti].r, mid + 1, tr, address);
	} else {
		if (slot[ti].l == 0) {
			Node ret;
			ret.address = 0;
			ret.size = 0;
			ret.l = 0;
			ret.r = 0;
			return ret;
		}
		return get1(slot[ti].l, tl, mid, address);
	}
}

void remove(uint32 address) {
	remove1(1, 0, (uint32) KERNEL_HEAP_MAX - 1, address);
}

void remove1(uint32 ti, uint32 tl, uint32 tr, uint32 address) {
	if (tl == tr) {
		if (address != slot[ti].address) {
			panic("Wrong Node to remove!");
		}
		slot[ti].address = 0;
		slot[ti].size = 0;
		slot[ti].l = 0;
		slot[ti].r = 0;
		return;
	}
	uint32 mid = ((unsigned long long) tl + tr) / 2;
	if (mid < address) {
		// MUST have right
		if (slot[ti].r == 0) {
			//cprintf("\n%u %u \n", address, slot[ti].address);
			panic("No right node!");
			//return;
		}
		remove1(slot[ti].r, mid + 1, tr, address);
		if (slot[slot[ti].r].address == 0) {
			slot[ti].r = 0;
		}
	} else {
		// MUST have right
		if (slot[ti].l == 0) {
			panic("No left node!");
			//return;
		}
		remove1(slot[ti].l, tl, mid, address);
		if (slot[slot[ti].l].address == 0) {
			slot[ti].l = 0;
		}
	}
	if (slot[ti].l == 0) {
		slot[ti].address = slot[slot[ti].r].address;
		slot[ti].size = slot[slot[ti].r].size;
	} else if (slot[ti].r == 0) {
		slot[ti].address = slot[slot[ti].l].address;
		slot[ti].size = slot[slot[ti].l].size;
	} else {
		slot[ti] = merge(slot[ti], slot[slot[ti].l], slot[slot[ti].r]);
	}
	if ((slot[ti].l && slot[slot[ti].l].size > slot[ti].size) || (slot[ti].r && slot[slot[ti].r].size > slot[ti].size)) {
		panic("\nti %u l %u r %u size %u l_size %u r_size %u\n", ti, slot[ti].l, slot[ti].r, slot[ti].size, slot[slot[ti].l].size, slot[slot[ti].r].size);
	}
}

//void remove_first_bigger(uint32 req_size) {
//	remove_first_bigger1(1, 0, (uint32) KERNEL_HEAP_MAX - 1, req_size);
//}
//
//// Note: get_first_bigger must have return a valid address if called with the same req_size
//void remove_first_bigger1(uint32 ti, uint32 tl, uint32 tr, uint32 req_size) {
////	cprintf("left and right: %u %u \n", slot[ti].l, slot[ti].r);
////	cprintf("req_size and size: %u %u\n", req_size, slot[ti].size);
////	cprintf("ti: %u, tl: %u, tr: %u\n", ti, tl, tr);
//	//cprintf("ti: %u, l: %u, r: %u\n", ti, slot[ti].l, slot[ti].r);
//
//	if (tl == tr) {
//		if (req_size < slot[ti].size) {
//			panic("Wrong Node to remove!");
//		}
//		slot[ti].address = 0;
//		slot[ti].size = 0;
//		slot[ti].l = 0;
//		slot[ti].r = 0;
//		return;
//	}
//
//	uint32 mid = ((unsigned long long) tl + tr) / 2;
//	if (slot[ti].l != 0 && slot[ti].r != 0) {	// has both left and right
//		if (slot[slot[ti].l].size >= req_size) {
//
//			remove_first_bigger1(slot[ti].l, tl, mid, req_size);
//			if (slot[slot[ti].l].address == 0) {
//				slot[ti].l = 0;
//				slot[ti].address = slot[slot[ti].r].address;
//				slot[ti].size = slot[slot[ti].r].size;
//			}
//
//		} else if (slot[slot[ti].r].size >= req_size) {
//
//			remove_first_bigger1(slot[ti].r, mid + 1, tr, req_size);
//			if (slot[slot[ti].r].address == 0) {
//				slot[ti].r = 0;
//				slot[ti].address = slot[slot[ti].l].address;
//				slot[ti].size = slot[slot[ti].l].size;
//			}
//		}
//
//	} else if (slot[ti].l != 0) {	// has only left
//		if (slot[slot[ti].l].size >= req_size) {
//			remove_first_bigger1(slot[ti].l, tl, mid, req_size);
//			if (slot[slot[ti].l].address == 0) {
//				slot[ti].l = 0;
//				slot[ti].address = 0;
//				slot[ti].size = 0;
//			}
//		}
//	} else if (slot[ti].r != 0) {	// has only right
//		if (slot[slot[ti].r].size >= req_size) {
//			remove_first_bigger1(slot[ti].r, mid + 1, tr, req_size);
//			if (slot[slot[ti].r].address == 0) {
//				slot[ti].r = 0;
//				slot[ti].address = 0;
//				slot[ti].size = 0;
//			}
//		}
//	} else if (slot[ti].size >= req_size) {	// no childern
//		slot[ti].address = 0;
//		slot[ti].size = 0;
//		return;
//	}
//}

// Actually gets first not smaller
Node get_first_bigger(uint32 req_size) {
	return get_first_bigger1(1, 0, (uint32) KERNEL_HEAP_MAX - 1, req_size);
}

// Note: Returns 0 if not found
Node get_first_bigger1(uint32 ti, uint32 tl, uint32 tr, uint32 req_size) {
	uint32 mid = ((unsigned long long) tl + tr) / 2;
//	if (req_size == 3 * (8 * 1024) / 2) {
//		cprintf("\nNode:  %u size: %u %u %u %u %u\n", ti, slot[ti].size, slot[ti].l, slot[ti].r, tl, tr);
//		if (tl == tr) {		// 524318 524272
//			cprintf("l_size, %u r_size %u\n", slot[slot[ti].l].size, slot[slot[ti].r].size);
//		}
//	}
//	if (tl == tr) {		// 524318 524272
//		cprintf("l_size, %u r_size %u\n", slot[slot[ti].l].size, slot[slot[ti].r].size);
//	}
	//cprintf("left and right: %u %u \n", slot[ti].l, slot[ti].r);
	//cprintf("req_size and size: %u %u\n", req_size, slot[ti].size);
	//cprintf("ti: %u, tl: %u, tr: %u\n", ti, tl, tr);
	//cprintf("get_first_bigger1 ti: %u, l: %u, r: %u\n", ti, slot[ti].l, slot[ti].r);

	if (slot[ti].l != 0 && slot[ti].r != 0) {	// has both left and right
		if (slot[slot[ti].l].size >= req_size) {
			return get_first_bigger1(slot[ti].l, tl, mid, req_size);
		} else if (slot[slot[ti].r].size >= req_size) {
			return get_first_bigger1(slot[ti].r, mid + 1, tr, req_size);
		}
	} else if (slot[ti].l != 0) {	// has only left
		if (slot[slot[ti].l].size >= req_size) {
			return get_first_bigger1(slot[ti].l, tl, mid, req_size);
		}
	} else if (slot[ti].r != 0) {	// has only right
//		if (req_size == 3 * (8 * 1024) / 2)
//			cprintf("\n%u\n", slot[slot[ti].r].size);
		if (slot[slot[ti].r].size >= req_size) {
			//cprintf("Right: %u\n", slot[slot[ti].r].size);
			return get_first_bigger1(slot[ti].r, mid + 1, tr, req_size);
		}
	} else if (slot[ti].size >= req_size) {	// no childern
		//cprintf("returned slot address: %u\n", slot[ti].address);
		return slot[ti];
	}
//	if (req_size == 3 * (8 * 1024) / 2)
//		cprintf("5555555555555555555");
	Node ret;
	ret.address = 0;
	ret.size = 0;
	ret.l = 0;
	ret.r = 0;
	//cprintf("returned slot address: %u\n", ret.address);
	return ret;
}
uint32 FREE_PAGES_COUNT;
//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
    //TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
    // Write your code here, remove the panic and write your code

    if (initSizeToAllocate > (daLimit - daStart)) {
        panic("Kernel heap initialization failed: initial size exceeds limit.");
        return -1;
    }

    if (daStart % PAGE_SIZE != 0 || daLimit % PAGE_SIZE != 0 || (daStart + initSizeToAllocate) % PAGE_SIZE != 0)
    {
        panic("some address is invalid");
        return -1;
    }


    for (uint32 addr = daStart; addr < daStart + initSizeToAllocate; addr += PAGE_SIZE)
    {
        struct FrameInfo *frame_alloc = NULL;
        int allocation_success = allocate_frame(&frame_alloc);

        if (allocation_success == E_NO_MEM)
        {
            panic("kheap initialization frames allocation failed Insufficient memory\n");
            return -1;
        }

        int mapping_success = map_frame(ptr_page_directory, frame_alloc, addr, (PERM_PRESENT | PERM_USER | PERM_WRITEABLE));

        if (mapping_success == E_NO_MEM)
        {
            panic("kheap initialization frames mapping failed Insufficient memory\n");
            return -1;
        }
        reverse_mapping[to_physical_address(frame_alloc) >> 12] = addr;
    }

    BLOCK_ALLOCATION_START = (void*)daStart;
    BREAK = (void*)(daStart + initSizeToAllocate);
    HARD_LIMIT = (void*)daLimit;
    PAGE_ALLOCATION_START = (void*)((int) HARD_LIMIT + PAGE_SIZE);
    //FREE_PAGES_COUNT = (KERNEL_HEAP_MAX - (uint32)PAGE_ALLOCATION_START) / PAGE_SIZE;
    // Initialize the dynamic allocator to manage memory within daStart to daLimit
    initialize_dynamic_allocator(daStart, initSizeToAllocate);
    return 0;
    // panic("initialize_kheap_dynamic_allocator() is not implemented yet...!!");
}

void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING==========
	//return (void*)-1 ;
	//====================================================

	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	// Write your code here, remove the panic and write your code
	//panic("sbrk() is not implemented yet...!!");

	if (numOfPages == 0)
	{
		return (void*)BREAK;
	}

	uint32 new_brk_pos = (uint32)BREAK + (numOfPages * PAGE_SIZE);

	if (new_brk_pos > (uint32)HARD_LIMIT)
	{
		return (void*)-1;
	}

	// allocating and mapping new pages
	for (uint32 addr = (uint32)BREAK; addr < new_brk_pos; addr += PAGE_SIZE)
	{
		struct FrameInfo *frame_alloc = NULL;
		int allocation_success = allocate_frame(&frame_alloc);

		if (allocation_success == E_NO_MEM)
		{
			panic("sbrk frames allocation failed Insufficient memory\n");
			return (void*)-1;
		}

		int mapping_success = map_frame(ptr_page_directory, frame_alloc, addr, (PERM_PRESENT | PERM_USER | PERM_WRITEABLE));

		if (mapping_success == E_NO_MEM)
		{
			panic("sbrk frames mapping failed Insufficient memory\n");
			return (void*)-1;
		}
		reverse_mapping[to_physical_address(frame_alloc) >> 12] = addr;
	}

	uint32 prev_brk = (uint32)BREAK;
	BREAK = (void*)new_brk_pos;

	// uint32* new_end_block_pos = (uint32*)(new_brk_pos - sizeof(uint32));
	// *new_end_block_pos = 1;

	// set_block_data((void*)prev_brk, (new_brk_pos - prev_brk), 1);
	// free_block((void*)prev_brk);
	return (void*)prev_brk;
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* slow_kmalloc(uint32 size) {
	if (!isKHeapPlacementStrategyFIRSTFIT()) {
		return NULL;
	}
	if (size <= PAGE_SIZE / 2) {
		return alloc_block_FF(size);
	}
	if (MemFrameLists.free_frame_list.size < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE) {
		return NULL;
	}
	uint32 r = (uint32) PAGE_ALLOCATION_START;
	uint32 l = r;
	uint32 cnt = 0;
	while (r + PAGE_SIZE - 1 < KERNEL_HEAP_MAX) { // KHM is a constant
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
					map_frame(ptr_page_directory, frame, l, PERM_WRITEABLE);
					l += PAGE_SIZE;
				}
				//get_page_table(ptr_page_directory, r, &page_table);
				page_table[PTX(r)] |= 1 << 9; // 11 10 9 available bits // use 9th bit as "segment end" flag
				return (void*) tl;
			}
			r += PAGE_SIZE;
		}
	}
	return NULL; // No suitable slot for whatever reason.
}

void* kmalloc(unsigned int size) {
	if (enable_fast_page_allocation == 0) {
		return slow_kmalloc(size);
	}
	numOfKheapVACalls++;
	if (!isKHeapPlacementStrategyFIRSTFIT()) {
		return NULL;
	}
	if (size <= PAGE_SIZE / 2) {
		return alloc_block_FF(size);
	}
	if (MemFrameLists.free_frame_list.size < ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE) {
		return NULL;
	}
	//panic("%u %u\n", (uint32) PAGE_ALLOCATION_START,(uint32) ACTUAL_START);

	if (!seg_tree_initialized) {
		for (int i = 0; i < 100000; i++) {
			slot[i].address = 0;
			slot[i].size = 0;
			slot[i].l = 0;
			slot[i].r = 0;
		}
		node_counter = 2;
		add((uint32) PAGE_ALLOCATION_START, KERNEL_HEAP_MAX - (uint32) PAGE_ALLOCATION_START);
		seg_tree_initialized = 1;
	}
	size = ROUNDUP(size, PAGE_SIZE);
	Node res = get_first_bigger(size);
	uint32 l = res.address;
	//cprintf("\nAddress: %u Size: %u\n", res.address, size);
	if (l == 0) {
		//cprintf("\nSize:: %u\n", size);
		return 0;
	}
	//remove_first_bigger(size);
	remove(l);
	uint32 rem_size = res.size - size; // to add again
	uint32 r = l + size - 1;
	uint32 tl = l;
	//cprintf("\nL: %u, R: %u\n", l, r);
	uint32* page_table;
	get_page_table(ptr_page_directory, r, &page_table);
	//--------------------------------------------------------
//	if (page_table[PTX(res.address)] & PERM_PRESENT) {
//		panic("Returned address for segtree is already present!, %u %u %u\n", res.address, (uint32 )PAGE_ALLOCATION_START, size);
//	}
	//--------------------------------------------------------
	while (l <= r) {
		struct FrameInfo* frame;
		allocate_frame(&frame);
		map_frame(ptr_page_directory, frame, l, PERM_WRITEABLE);
		reverse_mapping[to_physical_address(frame) >> 12] = l;
		l += PAGE_SIZE;
	}
	if (PTX(r) != PTX(r - PAGE_SIZE + 1)) {
		panic("PTX(r) != PTX(r - PAGE_SIZE + 1)");
	}
	page_table[PTX(r)] |= 1 << 9; // 11 10 9 available bits // use 9th bit as "segment end" flag
	if (ROUNDDOWN(rem_size, PAGE_SIZE) > 0 && ROUNDUP(r + 1, PAGE_SIZE) + ROUNDDOWN(rem_size, PAGE_SIZE) - 1 < KERNEL_HEAP_MAX) {
		add(ROUNDUP(r + 1, PAGE_SIZE), ROUNDDOWN(rem_size, PAGE_SIZE)); // Rounding for internal fragmentation
	}
	//cprintf("\ntl: %u\n", tl);
	return (void*) tl;

	/*uint32 r = (uint32)PAGE_ALLOCATION_START;
	 uint32 l = r;
	 uint32 cnt = 0;
	 while(r + PAGE_SIZE - 1 < KERNEL_HEAP_MAX) { // KHM is a constant
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
	 while(l <= r) {
	 struct FrameInfo* frame;
	 allocate_frame(&frame);
	 map_frame(ptr_page_directory, frame, l, PERM_WRITEABLE);
	 l += PAGE_SIZE;
	 }
	 //get_page_table(ptr_page_directory, r, &page_table);
	 page_table[PTX(r)] |= 1 << 9; // 11 10 9 available bits // use 9th bit as "segment end" flag
	 return (void*)tl;
	 }
	 r += PAGE_SIZE;
	 }
	 }*/
	//return NULL; // No suitable slot for whatever reason.
}


void slow_kfree(void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

	if ((uint32) virtual_address < KERNEL_HEAP_START || (uint32) virtual_address >= KERNEL_HEAP_MAX) {
		panic("ERROR - INVALID ADDRESS | OUT OF KERNEL_HEAP");
	} else if (virtual_address < sbrk(0)) {
		free_block(virtual_address);
	} else if (virtual_address >= HARD_LIMIT + PAGE_SIZE) {
		uint32* ptr_page_table = NULL;
		get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_page_table);
		if (ptr_page_table == NULL) {
			return;
		}
		if ((ptr_page_table[PTX(virtual_address)] & PERM_PRESENT) == 0) {
			return;
		}

		virtual_address = (void*) ROUNDDOWN((uint32 )virtual_address, PAGE_SIZE);

		while (virtual_address < (void*) KERNEL_HEAP_MAX) {

			unmap_frame(ptr_page_directory, (uint32) virtual_address);

			get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_page_table);

			if (ptr_page_table[PTX(virtual_address)] & (1 << 9)) {
				ptr_page_table[PTX(virtual_address)] &= ~(1 << 9);
				break;
			}

			virtual_address = (virtual_address) + PAGE_SIZE;
			if (virtual_address >= (void*) KERNEL_HEAP_MAX) {
				break;
			}
		}
	} else {
		panic("ERROR - INVALID ADDRESS | BETWEEN sbrk AND HARD_LIMIT");
	}

	/*
	 Unmap
	 - unmap_frame(ptr_page_directory, va)


	 Freeframe
	 - from va to pa
	 - to_frame_into(pa)
	 - free_frame(struct Frame_Info*)
	 */
}


void kfree(void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

	if (enable_fast_page_allocation == 0) {
		slow_kfree(virtual_address);
		return;
	}

	if ((uint32) virtual_address < KERNEL_HEAP_START || (uint32) virtual_address >= KERNEL_HEAP_MAX) {
		panic("ERROR - INVALID ADDRESS | OUT OF KERNEL_HEAP");
	} else if (virtual_address < sbrk(0)) {
		free_block(virtual_address);
	} else if (virtual_address >= HARD_LIMIT + PAGE_SIZE) {
		uint32* ptr_page_table = NULL;
		get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_page_table);
		uint32* page_table = NULL;
		get_page_table(ptr_page_directory, (uint32) PAGE_ALLOCATION_START, &page_table);
		//cprintf("\t 6Should be 0: %u \t", page_table[PTX(PTX((uint32)PAGE_ALLOCATION_START))] & PERM_PRESENT);
		if (ptr_page_table == NULL) {
			return;
		}
		if ((ptr_page_table[PTX(virtual_address)] & PERM_PRESENT) == 0) {
			return;
		}

		//virtual_address = (void*) ROUNDDOWN((uint32 )virtual_address, PAGE_SIZE);

		uint32 start = (uint32) virtual_address;
		uint32 next_start = start;

		// Note: next_start stops at first page after allocated slot
		while (virtual_address < (void*) KERNEL_HEAP_MAX) {

			unmap_frame(ptr_page_directory, (uint32) virtual_address);
			next_start += PAGE_SIZE;
			//virtual_address += PAGE_SIZE;
			get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_page_table);
			if (ptr_page_table[PTX(virtual_address)] & (1 << 9)) {
				ptr_page_table[PTX(virtual_address)] &= ~(1 << 9);
				break;
			}

			virtual_address = (virtual_address) + PAGE_SIZE;
		}
		if ((uint32) start == (uint32) PAGE_ALLOCATION_START + 2 * 1024 * 1024) {
			//cprintf("\n get_in_kfree(ptr_allocations[1]): %u \n", get((uint32) PAGE_ALLOCATION_START).address);
		}
		uint32 pt = start;
		//Note: pt stops at first connected free page
		while (pt - PAGE_SIZE >= (uint32) PAGE_ALLOCATION_START) {
			get_page_table(ptr_page_directory, pt - PAGE_SIZE, &ptr_page_table);
			if ((ptr_page_table[PTX(pt - PAGE_SIZE)] & PERM_PRESENT)) {
				break;
			}
			pt -= PAGE_SIZE;
		}
		//cprintf("\t 6Should be 0: %u \t", page_table[PTX(PTX((uint32)PAGE_ALLOCATION_START))] & PERM_PRESENT);
		//cprintf("\n %u %u %u \n", pt - (uint32) PAGE_ALLOCATION_START, (page_table[PTX((uint32 )PAGE_ALLOCATION_START)] & PERM_PRESENT), (page_table[PTX((uint32)PAGE_ALLOCATION_START + PAGE_SIZE)] & PERM_PRESENT));

		if (pt != start) {	// must merge with previous
			if ((uint32) start == (uint32) PAGE_ALLOCATION_START + 2 * 1024 * 1024) {
				cprintf("\n get_in_kfree(ptr_allocations[6]): %u , %u \n", get((uint32) pt).address, pt);
			}
			Node prev = get(pt);
			if (prev.address == 0) {
				//cprintf("\n%u \n", start - pt);
				panic("prev address can't be 0! %u %u %u\n", (start - pt), pt, (pt - (uint32 )PAGE_ALLOCATION_START));
			}
			//cprintf("\nprev: %u, cur: %u \n", prev.address, start);
			remove(pt);
		}
		uint32 pt2 = next_start;
		//Note: pt2 stops at start of next not free page
		while (pt2 + PAGE_SIZE - 1 <= KERNEL_HEAP_MAX) {
			get_page_table(ptr_page_directory, pt2 + 1, &ptr_page_table);
			if ((ptr_page_table[PTX(pt2 + 1)] & PERM_PRESENT)) {
				break;
			}
			pt2 += PAGE_SIZE;
		}
		if (pt2 != next_start) {
			Node next = get(next_start);
			if (next.address == 0) {
				//cprintf("\n%u\n", pt2 - start);
				panic("next address can't be 0!");
			}
			//cprintf("\nnext_size: %u \n", next.size);
			remove(next_start);
		}
//		uint32 size = pt2 - pt + PAGE_SIZE;
//		add(pt, size);
		//cprintf("\n %u %u \n", pt - (uint32)PAGE_ALLOCATION_START, (ptr_page_table[PTX((uint32)PAGE_ALLOCATION_START + PAGE_SIZE)] & PERM_PRESENT));
		add(pt, pt2 - pt);
		if (pt == (uint32) PAGE_ALLOCATION_START) {
			cprintf("\n actual_start_here: %u\n", get(pt).address);
		}
		//cprintf("\n%u\n", (pt2 - pt) / (8 * 1024));

	} else {
		panic("ERROR - INVALID ADDRESS | BETWEEN sbrk AND HARD_LIMIT");
	}

	/*
	 Unmap
	 - unmap_frame(ptr_page_directory, va)


	 Freeframe
	 - from va to pa
	 - to_frame_into(pa)
	 - free_frame(struct Frame_Info*)
	 */
}
unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	/*if(virtual_address >= KERNEL_HEAP_MAX){
	        return 0;
	    }
*/
	    uint32 *ptr_page_table = NULL;
	    get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	    if(ptr_page_table != NULL){
	        uint32 table_entry = ptr_page_table[PTX(virtual_address)];
	        if((table_entry & PERM_PRESENT) == 0) return 0;

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

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	uint32 frame_num = physical_address / PAGE_SIZE;
	uint32 offset = physical_address % PAGE_SIZE;

	struct FrameInfo *frame_info = to_frame_info(physical_address);
	if(frame_info->references == 0 || reverse_mapping[frame_num] == 0) return 0;

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
	if (virtual_address < BLOCK_ALLOCATION_START || virtual_address > (void*) KERNEL_HEAP_MAX) {

		return NULL;
	}
	if (virtual_address < PAGE_ALLOCATION_START && virtual_address > BREAK) {

		return NULL;
	}

	enable_fast_page_allocation = 0;
	if (virtual_address < BREAK) {
		if (new_size <= PAGE_SIZE / 2)
			return realloc_block_FF(virtual_address, new_size);

		void* ret = kmalloc(new_size);
		if (ret != NULL) {
			char* ptr = (char*) ret;
			char* old = (char*) virtual_address;
			uint32 old_size = get_block_size(virtual_address);
			for (uint32 i = 0; i + 8 < old_size; i++) // 8 for header & footer
				ptr[i] = old[i];
			free_block(virtual_address);
		}
		return ret;
	}

	// ELSE : ADDRESS IS AT THE PAGE ALLOCACTION AREA
	virtual_address = ROUNDDOWN(virtual_address, PAGE_SIZE);
	if (new_size <= PAGE_SIZE / 2) {
		void* ret = alloc_block_FF(new_size);
		if (ret != NULL) {
			char* ptr = (char*) ret;
			char* old = (char*) virtual_address;
			for (uint32 i = 0; i < new_size; i++)
				ptr[i] = old[i];
			kfree(virtual_address);
		}
		return ret;
	}

	uint32 l = (uint32) virtual_address;
	uint32 r = l;
	uint32* ptr_page_table = NULL;

	while (1) {
		get_page_table(ptr_page_directory, r, &ptr_page_table);
		if (ptr_page_table[PTX(r)] & (1 << 9))
			break;
		r += PAGE_SIZE;
	}

	uint32 cur_pages_count = (r - l + 1) / PAGE_SIZE;
	uint32 needed_pages_count = ROUNDUP(new_size, PAGE_SIZE) / PAGE_SIZE;

	if (cur_pages_count > needed_pages_count) {
		while (cur_pages_count > needed_pages_count) {
			get_page_table(ptr_page_directory, r, &ptr_page_table);
			unmap_frame(ptr_page_directory, r);
			ptr_page_table[PTX(r)] &= ~((1 << 9) | PERM_PRESENT);
			cur_pages_count--;
			r -= PAGE_SIZE;
		}
		get_page_table(ptr_page_directory, r, &ptr_page_table);
		ptr_page_table[PTX(r)] |= (1 << 9);
	} else if (cur_pages_count < needed_pages_count) {
		if (MemFrameLists.free_frame_list.size < needed_pages_count - cur_pages_count) {
			return NULL;

			uint32 new_r = r + PAGE_SIZE;
			uint32 free_after_cur = 0;
			while (new_r < (uint32) KERNEL_HEAP_MAX) {
				get_page_table(ptr_page_directory, new_r, &ptr_page_table);
				if (ptr_page_table[PTX(new_r)] & PERM_PRESENT)
					break;
				if (++free_after_cur + cur_pages_count == needed_pages_count)
					break;
				new_r += PAGE_SIZE;
			}

			if (cur_pages_count + free_after_cur == needed_pages_count) {
				get_page_table(ptr_page_directory, r, &ptr_page_table);
				ptr_page_table[PTX(r)] &= ~(1 << 9);

				r += PAGE_SIZE;
				while (r < new_r) {
					struct FrameInfo* frame;
					allocate_frame(&frame);
					map_frame(ptr_page_directory, frame, r, PERM_WRITEABLE);
				}

				get_page_table(ptr_page_directory, new_r, &ptr_page_table);
				ptr_page_table[PTX(new_r)] |= (1 << 9) | PERM_PRESENT;

				return virtual_address;
			}
		} else { // search for another place at the pages allocation area
			uint32 y = (uint32) PAGE_ALLOCATION_START;
			uint32 x = y;
			uint32 cnt = 0;
			while (y + PAGE_SIZE - 1 < KERNEL_HEAP_MAX) {
				get_page_table(ptr_page_directory, y, &ptr_page_table); // Can't find a `create` parameter
				if (ptr_page_table[PTX(y)] & PERM_PRESENT) {
					y += PAGE_SIZE;
					x = y;
					cnt = 0;
				} else {
					cnt++;
					if (cnt * PAGE_SIZE >= new_size) {
						uint32 tl = x;
						while (x <= y) {
							if (l <= r) {
								struct FrameInfo* frame = get_frame_info(ptr_page_directory, l, &ptr_page_table);
								map_frame(ptr_page_directory, frame, x, PERM_WRITEABLE);
								unmap_frame(ptr_page_directory, l);
								ptr_page_table[PTX(l)] &= ~(1 << 9);
								l += PAGE_SIZE;
							} else {
								struct FrameInfo* frame;
								allocate_frame(&frame);
								map_frame(ptr_page_directory, frame, x, PERM_WRITEABLE);
							}
							x += PAGE_SIZE;
						}
						get_page_table(ptr_page_directory, y, &ptr_page_table); // Can't find a `create` parameter

						ptr_page_table[PTX(y)] |= 1 << 9; // 11 10 9 available bits // use 9th bit as "segment end" flag

						return (void*) tl;
					}
					y += PAGE_SIZE;
				}
			}
			return NULL;
		}
	} else
		return virtual_address;

	return NULL;
}
