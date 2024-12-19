/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE) {
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE;
}
void setPageReplacmentAlgorithmCLOCK() {
	_PageRepAlgoType = PG_REP_CLOCK;
}
void setPageReplacmentAlgorithmFIFO() {
	_PageRepAlgoType = PG_REP_FIFO;
}
void setPageReplacmentAlgorithmModifiedCLOCK() {
	_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;
}
/*2018*/void setPageReplacmentAlgorithmDynamicLocal() {
	_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;
}
/*2021*/void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps) {
	_PageRepAlgoType = PG_REP_NchanceCLOCK;
	page_WS_max_sweeps = PageWSMaxSweeps;
}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE) {
	return _PageRepAlgoType == LRU_TYPE ? 1 : 0;
}
uint32 isPageReplacmentAlgorithmCLOCK() {
	if (_PageRepAlgoType == PG_REP_CLOCK)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmFIFO() {
	if (_PageRepAlgoType == PG_REP_FIFO)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmModifiedCLOCK() {
	if (_PageRepAlgoType == PG_REP_MODIFIEDCLOCK)
		return 1;
	return 0;
}
/*2018*/uint32 isPageReplacmentAlgorithmDynamicLocal() {
	if (_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL)
		return 1;
	return 0;
}
/*2021*/uint32 isPageReplacmentAlgorithmNchanceCLOCK() {
	if (_PageRepAlgoType == PG_REP_NchanceCLOCK)
		return 1;
	return 0;
}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt) {
	_EnableModifiedBuffer = enableIt;
}
uint8 isModifiedBufferEnabled() {
	return _EnableModifiedBuffer;
}

void enableBuffering(uint32 enableIt) {
	_EnableBuffering = enableIt;
}
uint8 isBufferingEnabled() {
	return _EnableBuffering;
}

void setModifiedBufferLength(uint32 length) {
	_ModifiedBufferLength = length;
}
uint32 getModifiedBufferLength() {
	return _ModifiedBufferLength;
}

//===============================
// FAULT HANDLERS
//===============================

//==================
// [1] MAIN HANDLER:
//==================
/*2022*/
uint32 last_eip = 0;
uint32 before_last_eip = 0;
uint32 last_fault_va = 0;
uint32 before_last_fault_va = 0;
int8 num_repeated_fault = 0;

struct Env* last_faulted_env = NULL;
void fault_handler(struct Trapframe *tf) {

	// cprintf("%~\n1: BEFORE CHECKING\n");
	/******************************************************/
	// Read processor's CR2 register to find the faulting address
	uint32 fault_va = rcr2();
	//	cprintf("\n************Faulted VA = %x************\n", fault_va);
	//	print_trapframe(tf);
	/******************************************************/

	//If same fault va for 3 times, then panic
	//UPDATE: 3 FAULTS MUST come from the same environment (or the kernel)
	struct Env* cur_env = get_cpu_proc();
	if (last_fault_va == fault_va && last_faulted_env == cur_env) {
		num_repeated_fault++;
		if (num_repeated_fault == 3) {
			print_trapframe(tf);
			panic("Failed to handle fault! fault @ at va = %x from eip = %x causes va (%x) to be faulted for 3 successive times\n", before_last_fault_va, before_last_eip, fault_va);
		}
	} else {
		before_last_fault_va = last_fault_va;
		before_last_eip = last_eip;
		num_repeated_fault = 0;
	}
	last_eip = (uint32) tf->tf_eip;
	last_fault_va = fault_va;
	last_faulted_env = cur_env;
	/******************************************************/
	//2017: Check stack overflow for Kernel
	int userTrap = 0;
	if ((tf->tf_cs & 3) == 3) {
		userTrap = 1;
	}
	if (!userTrap) {
		struct cpu* c = mycpu();
		//cprintf("trap from KERNEL\n");
		if (cur_env && fault_va >= (uint32) cur_env->kstack && fault_va < (uint32) cur_env->kstack + PAGE_SIZE)
			panic("User Kernel Stack: overflow exception!");
		else if (fault_va >= (uint32) c->stack && fault_va < (uint32) c->stack + PAGE_SIZE)
			panic("Sched Kernel Stack of CPU #%d: overflow exception!", c - CPUS);
#if USE_KHEAP
		if (fault_va >= KERNEL_HEAP_MAX)
			panic("Kernel: heap overflow exception!");
#endif
	}
	//2017: Check stack underflow for User
	else {
		//cprintf("trap from USER\n");
		if (fault_va >= USTACKTOP && fault_va < USER_TOP)
			panic("User: stack underflow exception!");
	}

	//get a pointer to the environment that caused the fault at runtime
	//cprintf("curenv = %x\n", curenv);
	struct Env* faulted_env = cur_env;
	if (faulted_env == NULL) {
		print_trapframe(tf);
		panic("faulted env == NULL!");
	}
	//check the faulted address, is it a table or not ?
	//If the directory entry of the faulted address is NOT PRESENT then
	if ((faulted_env->env_page_directory[PDX(fault_va)] & PERM_PRESENT) != PERM_PRESENT) {
		// we have a table fault =============================================================
		//		cprintf("[%s] user TABLE fault va %08x\n", curenv->prog_name, fault_va);
		//		print_trapframe(tf);

		faulted_env->tableFaultsCounter++;

		table_fault_handler(faulted_env, fault_va);
	} else {

		if (userTrap) {
			cprintf("THE FAULTED ADDRESS IS %x\n", fault_va);
			// cprintf("%~\n1: BEFORE CHECKING 2\n");
			/*============================================================================================*/
			//TODO: [PROJECT'24.MS2 - #08] [2] FAULT HANDLER I - Check for invalid pointers
			//(e.g. pointing to unmarked user heap page, kernel or wrong access rights),
			//your code is here
			uint32 permissions = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);
			cprintf("THE PERMS %x\n", permissions);

			bool is_in_kernel_limit = (fault_va >= USER_LIMIT);
			bool is_marked_page = ((permissions & PERM_MARKED) != 0 && (permissions & PERM_PRESENT) == 0);
			bool is_read_only = ((permissions & PERM_WRITEABLE) == 0 && (permissions & PERM_PRESENT) != 0);

			if (is_in_kernel_limit) {
				cprintf("is in kernel heap\n");
				env_exit();
			} else if (((permissions & PERM_MARKED) == 0 && (permissions & PERM_PRESENT) == 0) && fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) {
				cprintf("is not marked\n");
				env_exit();
			} else if (((permissions & PERM_WRITEABLE) == 0 && (permissions & PERM_PRESENT) != 0)) {
				cprintf("is read only\n");
				env_exit();
			}
			// cprintf("%~\n1: VALID POINTER\n");
			/*============================================================================================*/
		}

		/*2022: Check if fault due to Access Rights */
		int perms = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);
		if (perms & PERM_PRESENT)
			panic("Page @va=%x is exist! page fault due to violation of ACCESS RIGHTS\n", fault_va);
		/*============================================================================================*/

		// we have normal page fault =============================================================
		faulted_env->pageFaultsCounter++;

		//		cprintf("[%08s] user PAGE fault va %08x\n", curenv->prog_name, fault_va);
		//		cprintf("\nPage working set BEFORE fault handler...\n");
		//		env_page_ws_print(curenv);

		if (isBufferingEnabled()) {
			__page_fault_handler_with_buffering(faulted_env, fault_va);
		} else {
			//page_fault_handler(faulted_env, fault_va);
			page_fault_handler(faulted_env, fault_va);
		}
		//		cprintf("\nPage working set AFTER fault handler...\n");
		//		env_page_ws_print(curenv);

	}

	/*************************************************************/
	//Refresh the TLB cache
	tlbflush();
	/*************************************************************/
}

//=========================
// [2] TABLE FAULT HANDLER:
//=========================
void table_fault_handler(struct Env * curenv, uint32 fault_va) {
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32) fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//=========================
// [3] PAGE FAULT HANDLER:
//=========================
void page_fault_handler(struct Env * faulted_env, uint32 fault_va) {
#if USE_KHEAP
	struct WorkingSetElement *victimWSElement = NULL;
	uint32 wsSize = LIST_SIZE(&(faulted_env->page_WS_list));
#else
	int iWS =faulted_env->page_last_WS_index;
	uint32 wsSize = env_page_ws_get_size(faulted_env);
#endif

	if (wsSize < (faulted_env->page_WS_max_size)) {
		//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
		//TODO: [PROJECT'24.MS2 - #09] [2] FAULT HANDLER I - Placement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");

		struct FrameInfo* frame = NULL;
		int allocation_status = allocate_frame(&frame);
		if (allocation_status == E_NO_MEM) {
			panic("page_fault_handler: error in allocating frame\n");
		}

		int mapping_status = map_frame(faulted_env->env_page_directory, frame, fault_va, (PERM_PRESENT | PERM_USER | PERM_WRITEABLE));

		if (mapping_status == E_NO_MEM) {
			panic("page_fault_handler: error in mapping frame\n");
		}
		int read = pf_read_env_page(faulted_env, (void *) fault_va);

		if (read == E_PAGE_NOT_EXIST_IN_PF) {
			if ((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)) {

				struct WorkingSetElement *element = env_page_ws_list_create_element(faulted_env, fault_va);
				LIST_INSERT_TAIL(&(faulted_env->page_WS_list), element);

				wsSize++;
				if (wsSize == (faulted_env->page_WS_max_size)) {
					faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
				} else {
					faulted_env->page_last_WS_element = NULL;

				}
			} else {
				cprintf("\n Gotch motherfucker \n");
				env_exit();
			}
		} else {
			struct WorkingSetElement *element = env_page_ws_list_create_element(faulted_env, fault_va);
			LIST_INSERT_TAIL(&(faulted_env->page_WS_list), element);

			wsSize++;
			if (wsSize == (faulted_env->page_WS_max_size)) {
				faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
			} else {
				faulted_env->page_last_WS_element = NULL;

			}
		}

	} else {
		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
		//TODO: [PROJECT'24.MS3] [2] FAULT HANDLER II - Replacement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() Replacement is not implemented yet...!!");
		/*
		 * LOGIC: which got less sweeps to reach the nth first
		 */
		int max_size = faulted_env->page_WS_max_size;
		struct WorkingSetElement *iterator = faulted_env->page_last_WS_element;
		struct WorkingSetElement *last_ws_element = LIST_LAST(&(faulted_env->page_WS_list));
		struct WorkingSetElement *replaced = NULL;
		uint32 n = page_WS_max_sweeps;
		uint32 perms;
		bool neg = (n < 0 ? 1 : 0);
		n *= (neg ? -1 : 1);

		int min_difference = n + 10;
		int temp = max_size;

		while (max_size--) {
			perms = pt_get_page_permissions(faulted_env->env_page_directory, iterator->virtual_address);
			if (perms & PERM_USED) {
				// +1 for reseting to 0 and n+1 for extra chance and moving to n
				if (neg && (perms & PERM_MODIFIED)) {
					if ((n + 2) < min_difference) {
						replaced = iterator;
						min_difference = n + 2;
					}
				}
				// n + 1 for reseting to 0 + moving to n
				else if ((n + 1) < min_difference) {
					replaced = iterator;
					min_difference = (n + 1);
				}
			} else {
				if (neg && (perms & PERM_MODIFIED)) {
					if ((n + 1) - iterator->sweeps_counter < min_difference) {
						replaced = iterator;
						min_difference = (n + 1) - iterator->sweeps_counter;
					}
				} else if (n - iterator->sweeps_counter < min_difference) {
					replaced = iterator;
					min_difference = n - iterator->sweeps_counter;
				}
			}
			if (iterator == last_ws_element) {
				iterator = LIST_FIRST(&(faulted_env->page_WS_list));
			} else {
				iterator = LIST_NEXT(iterator);
			}
		}

		// iterate to modify the number of sweeps
		struct WorkingSetElement *current = faulted_env->page_last_WS_element;
		bool passed_replaced = 0;
		while (temp--) {

			if (current == replaced && min_difference <= 1)
				break;

			perms = pt_get_page_permissions(faulted_env->env_page_directory, current->virtual_address);
			if (perms & PERM_USED) {
				// reset used to 0
				pt_set_page_permissions(faulted_env->env_page_directory, current->virtual_address, 0, PERM_USED);
				if (min_difference <= 1) {
					current->sweeps_counter = 0;
				} else {
					if (passed_replaced) {
						current->sweeps_counter = (min_difference - 2);
					} else {
						current->sweeps_counter = (min_difference - 1);
					}
				}
			} else {
				if (min_difference <= 1) {
					current->sweeps_counter += 1;
				} else {
					current->sweeps_counter += (min_difference - (passed_replaced));
				}
			}
			if (current == replaced)
				passed_replaced = 1;

			if (current == last_ws_element) {
				current = LIST_FIRST(&(faulted_env->page_WS_list));
			} else {
				current = LIST_NEXT(current);
			}
		}

		// check if replaced modified to update it on disk
		perms = pt_get_page_permissions(faulted_env->env_page_directory, replaced->virtual_address);
		if (perms & PERM_MODIFIED) {
			struct FrameInfo *frame_info = get_frame_info(faulted_env->env_page_directory, replaced->virtual_address, NULL);
			if (frame_info != NULL) {
				pf_update_env_page(faulted_env, replaced->virtual_address, frame_info);
			}
		}
		// Remap the frame to the faulted virtual address (fault_va)
		struct FrameInfo *frame_info = get_frame_info(faulted_env->env_page_directory, replaced->virtual_address, NULL);
		map_frame(faulted_env->env_page_directory, frame_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);
		// unmap the frame from current va
		unmap_frame(faulted_env->env_page_directory, replaced->virtual_address);

		// read from page file
		if (pf_read_env_page(faulted_env, (void *) fault_va) == E_PAGE_NOT_EXIST_IN_PF) {
			// If the page is not in the page file, check if it's a valid memory region
			if ((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)) {
				// Update the replaced element to point to the new faulted virtual address
				replaced->virtual_address = fault_va;
				replaced->sweeps_counter = 0;
				pt_set_page_permissions(faulted_env->env_page_directory, fault_va, PERM_USED | PERM_USER | PERM_WRITEABLE | PERM_MARKED | PERM_PRESENT, 0);
			} else {
				env_exit();
			}
		} else {
			// Update the replaced element to point to the new faulted virtual address
			replaced->virtual_address = fault_va;
			replaced->sweeps_counter = 0;
			pt_set_page_permissions(faulted_env->env_page_directory, fault_va, PERM_USED | PERM_USER | PERM_WRITEABLE | PERM_MARKED | PERM_PRESENT, 0);
		}

		// set last to be after the last inserted element
		last_ws_element = LIST_LAST(&(faulted_env->page_WS_list));
		if (replaced == last_ws_element) {
			faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
		} else {
			faulted_env->page_last_WS_element = LIST_NEXT(replaced);
		}

	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va) {
	//[PROJECT] PAGE FAULT HANDLER WITH BUFFERING
	// your code is here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");
}


/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

//#include "trap.h"
//#include <kern/proc/user_environment.h>
//#include "../cpu/sched.h"
//#include "../disk/pagefile_manager.h"
//#include "../mem/memory_manager.h"
//
////2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
//// 0 means don't bypass the PAGE FAULT
//uint8 bypassInstrLength = 0;
//
////===============================
//// REPLACEMENT STRATEGIES
////===============================
////2020
//void setPageReplacmentAlgorithmLRU(int LRU_TYPE) {
//	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
//	_PageRepAlgoType = LRU_TYPE;
//}
//void setPageReplacmentAlgorithmCLOCK() {
//	_PageRepAlgoType = PG_REP_CLOCK;
//}
//void setPageReplacmentAlgorithmFIFO() {
//	_PageRepAlgoType = PG_REP_FIFO;
//}
//void setPageReplacmentAlgorithmModifiedCLOCK() {
//	_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;
//}
///*2018*/void setPageReplacmentAlgorithmDynamicLocal() {
//	_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;
//}
///*2021*/void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps) {
//	_PageRepAlgoType = PG_REP_NchanceCLOCK;
//	page_WS_max_sweeps = PageWSMaxSweeps;
//}
//
////2020
//uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE) {
//	return _PageRepAlgoType == LRU_TYPE ? 1 : 0;
//}
//uint32 isPageReplacmentAlgorithmCLOCK() {
//	if (_PageRepAlgoType == PG_REP_CLOCK)
//		return 1;
//	return 0;
//}
//uint32 isPageReplacmentAlgorithmFIFO() {
//	if (_PageRepAlgoType == PG_REP_FIFO)
//		return 1;
//	return 0;
//}
//uint32 isPageReplacmentAlgorithmModifiedCLOCK() {
//	if (_PageRepAlgoType == PG_REP_MODIFIEDCLOCK)
//		return 1;
//	return 0;
//}
///*2018*/uint32 isPageReplacmentAlgorithmDynamicLocal() {
//	if (_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL)
//		return 1;
//	return 0;
//}
///*2021*/uint32 isPageReplacmentAlgorithmNchanceCLOCK() {
//	if (_PageRepAlgoType == PG_REP_NchanceCLOCK)
//		return 1;
//	return 0;
//}
//
////===============================
//// PAGE BUFFERING
////===============================
//void enableModifiedBuffer(uint32 enableIt) {
//	_EnableModifiedBuffer = enableIt;
//}
//uint8 isModifiedBufferEnabled() {
//	return _EnableModifiedBuffer;
//}
//
//void enableBuffering(uint32 enableIt) {
//	_EnableBuffering = enableIt;
//}
//uint8 isBufferingEnabled() {
//	return _EnableBuffering;
//}
//
//void setModifiedBufferLength(uint32 length) {
//	_ModifiedBufferLength = length;
//}
//uint32 getModifiedBufferLength() {
//	return _ModifiedBufferLength;
//}
//
////===============================
//// FAULT HANDLERS
////===============================
//
//uint32 last_fault_va = 0;
//int8 num_repeated_fault  = 0;
//struct Env * last_faulted_env = NULL;
//void fault_handler(struct Trapframe *tf)
//{
//	int userTrap = 0;
//	if ((tf->tf_cs & 3) == 3) {
//		userTrap = 1;
//	}
//	uint32 fault_va;
//
//	// Read processor's CR2 register to find the faulting address
//	fault_va = rcr2();
//
//	//	cprintf("Faulted VA = %x\n", fault_va);
//	//	print_trapframe(tf);
//
//	/******************************************************/
//	/*2022*///If same fault va for 3 times, then panic
//	struct Env* curenv = get_cpu_proc();
//	if (last_fault_va == fault_va && last_faulted_env == curenv)
//	{
//		num_repeated_fault++ ;
//		if (num_repeated_fault == 170)
//		{
//			print_trapframe(tf);
//			panic("Failed to handle fault at va=%u: same va is faulted for 3 successive times, user? %d\n", (uint32)fault_va, userTrap);
//		}
//	}
//	else
//	{
//		num_repeated_fault = 0;
//	}
//
//	last_fault_va = fault_va ;
//	last_faulted_env = curenv;
//	/******************************************************/
//	//2017: Check stack overflow for Kernel
//	if (!userTrap)
//	{
//		//cprintf("trap from KERNEL\n");
//		if (fault_va < KERN_STACK_TOP - KERNEL_STACK_SIZE && fault_va >= USER_LIMIT)
//			panic("Kernel: stack overflow exception!");
//#if USE_KHEAP
//		if (fault_va >= KERNEL_HEAP_MAX)
//			panic("Kernel: heap overflow exception!");
//#endif
//	}
//	//2017: Check stack underflow for User
//	else
//	{
//		//cprintf("trap from USER\n");
//		if (fault_va >= USTACKTOP && fault_va < USER_TOP)
//			panic("User: stack underflow exception!");
//	}
//
//	//get a pointer to the environment that caused the fault at runtime
//	struct Env* faulted_env = curenv;
//
//	//check the faulted address, is it a table or not ?
//	//If the directory entry of the faulted address is NOT PRESENT then
//	if ( (faulted_env->env_page_directory[PDX(fault_va)] & PERM_PRESENT) != PERM_PRESENT)
//	{
//		cprintf("\n Table fault \n");
//		faulted_env->tableFaultsCounter ++ ;
//		table_fault_handler(faulted_env, fault_va);
//	}
//	else
//	{
//		if(userTrap)
//		{
//			/*============================================================================================*/
//			//TODO: [PROJECT'23.MS2 - #13] [3] PAGE FAULT HANDLER - Check for invalid pointers
//			//(e.g. pointing to unmarked user heap page, kernel or wrong access rights),
//			//your code is here
//		   uint32 *page_table = NULL;
//		   get_page_table(curenv->env_page_directory,fault_va,&page_table);
//           int index=PTX(fault_va);
//           if(page_table[index]&PERM_PRESENT){
//        	   if ((page_table[index] & PERM_USER)!=PERM_USER) {
//        		   cprintf("a \n");
//        	  	sched_kill_env(curenv->env_id);
//        	   }
//        	   if((page_table[index] & PERM_WRITEABLE)!=PERM_WRITEABLE){
//        		   cprintf("aa \n");
//        	   	sched_kill_env(curenv->env_id);
//        	   }
//           }
//           else{
//        	   if(fault_va<=USER_HEAP_MAX && fault_va >= USER_HEAP_START){
//        		   if((page_table[index] & PERM_AVAILABLE)!= PERM_AVAILABLE){
//        			   cprintf("aaa \n");
//        		       sched_kill_env(curenv->env_id);
//        		   }
//        	   }
//           }
//			/*============================================================================================*/
//		}
//
//		//cprintf("\n After User trap (we know it ain't) \n");
//
//		/*2022: Check if fault due to Access Rights */
//		int perms = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);
//		if (perms & PERM_PRESENT)
//			panic("Page @va=%x is exist! page fault due to violation of ACCESS RIGHTS\n", fault_va) ;
//		// we have normal page fault =============================================================
//		faulted_env->pageFaultsCounter ++ ;
//
//		if(isBufferingEnabled())
//		{
//			__page_fault_handler_with_buffering(faulted_env, fault_va);
//		}
//		else
//		{
//			page_fault_handler(faulted_env, fault_va);
//		}
//	}
//
//	/*************************************************************/
//	//Refresh the TLB cache
//	tlbflush();
//	/*************************************************************/
//
//}
//
////Handle the table fault
//void table_fault_handler(struct Env * curenv, uint32 fault_va) {
//	//panic("table_fault_handler() is not implemented yet...!!");
//	//Check if it's a stack page
//	uint32* ptr_table;
//#if USE_KHEAP
//	{
//		ptr_table = create_page_table(curenv->env_page_directory, (uint32) fault_va);
//	}
//#else
//	{
//		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
//	}
//#endif
//}
//
////Handle the page fault
//
//void page_fault_handler(struct Env * curenv, uint32 fault_va) {
//#if USE_KHEAP
//	struct WorkingSetElement *victimWSElement = NULL;
//	uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
//#else
//	int iWS =curenv->page_last_WS_index;
//	uint32 wsSize = env_page_ws_get_size(curenv);
//#endif
//
//	if (isPageReplacmentAlgorithmFIFO()) {
//		if (wsSize < (curenv->page_WS_max_size)) {
//			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
//			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
//			// Write your code here, remove the panic and write your code
//
//			struct FrameInfo *new_frame;
//			int ret = allocate_frame(&new_frame);
//			if (ret == 0) {
//				ret = map_frame(curenv->env_page_directory, new_frame, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);
//				if (ret == 0) {
//					int ret3 = pf_read_env_page(curenv, (void*) fault_va);
//					//the page not exist in page file
//					if (ret3 == E_PAGE_NOT_EXIST_IN_PF) {
//						if ((fault_va < USER_HEAP_START || fault_va > USER_HEAP_MAX) && (fault_va > USTACKTOP || fault_va < USTACKBOTTOM)) {
//							cprintf("\n HERE: %u \n", (uint32)fault_va);
//							sched_kill_env(curenv->env_id);
//						}
//					}
//					struct WorkingSetElement*new_elm = env_page_ws_list_create_element(curenv, fault_va);
//					LIST_INSERT_TAIL(&(curenv->page_WS_list), new_elm);
//					if ((curenv->page_WS_max_size) == LIST_SIZE(&(curenv->page_WS_list))) {
//						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
//					} else {
//						curenv->page_last_WS_element = NULL;
//					}
//				}
//			}
//
//		} else {
//			//refer to the project presentation and documentation for details
//			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
//			// Write your code here, remove the panic and write your code
//			//panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
//
//			bool last = 0;          //check if it's last elm in tail of list
//			if (curenv->page_last_WS_element == LIST_LAST(&(curenv->page_WS_list))) {
//				last = 1;
//			}
//
//			struct FrameInfo *new_frame;
//			int ret = allocate_frame(&new_frame);
//			if (ret == 0) {
//				ret = map_frame(curenv->env_page_directory, new_frame, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);
//				if (ret == 0) {
//					int ret3 = pf_read_env_page(curenv, (void*) fault_va);
//					//the page not exist in page file
//					if (ret3 == E_PAGE_NOT_EXIST_IN_PF) {
//						if ((fault_va < USER_HEAP_START || fault_va > USER_HEAP_MAX) && (fault_va > USTACKTOP || fault_va < USTACKBOTTOM)) {
//							sched_kill_env(curenv->env_id);
//						}
//					}
//
//					struct WorkingSetElement* victim = curenv->page_last_WS_element;
//					uint32 *ptr_page_table = NULL;
//					struct FrameInfo *old_frame = get_frame_info(curenv->env_page_directory, victim->virtual_address, &ptr_page_table);
//					get_page_table(curenv->env_page_directory, victim->virtual_address, &ptr_page_table);
//					int index = PTX(victim->virtual_address);
//					if ((ptr_page_table[index] & PERM_MODIFIED) == PERM_MODIFIED) {
//						pf_update_env_page(curenv, victim->virtual_address, old_frame);
//					}
//
//					env_page_ws_invalidate(curenv, victim->virtual_address);
//					unmap_frame(curenv->env_page_directory, victim->virtual_address);
//					struct WorkingSetElement*new_elm = env_page_ws_list_create_element(curenv, fault_va);
//					if (last == 1) {
//						LIST_INSERT_TAIL(&(curenv->page_WS_list), new_elm);
//					} else {
//						LIST_INSERT_BEFORE(&(curenv->page_WS_list), curenv->page_last_WS_element, new_elm);
//					}
//
//				}
//			}
//
//			if (last == 1) {
//				curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
//			}
//
//		}
//	}
//	//panic("Placement Should be FIFO! \n");
//	//else {
//
////refer to the project presentation and documentation for details
//// FIRST WE NEED TO CHECK IF THE FAULTED VA IN THE SECONDE LIST OR NOT
////fault_va=ROUNDDOWN(fault_va,PAGE_SIZE);
//
////		uint32*page_table = NULL;
////		get_page_table(curenv->env_page_directory, fault_va, &page_table);
////		struct FrameInfo*check_frame = get_frame_info(curenv->env_page_directory, fault_va, &page_table);
////		uint32*page_table_2 = NULL;
////		get_page_table(curenv->env_page_directory, ROUNDDOWN(fault_va, PAGE_SIZE), &page_table_2);
////		struct FrameInfo*check_frame_2 = get_frame_info(curenv->env_page_directory, ROUNDDOWN(fault_va, PAGE_SIZE), &page_table_2);
//////env_page_ws_print(curenv);
////		if (check_frame_2 != NULL)
////			check_frame = check_frame_2;
////
////		struct WorkingSetElement *exist = NULL;
////		if (check_frame != NULL) {
////			exist = check_frame->element;
////		}
////
////		if (exist != NULL) {
////			LIST_REMOVE(&(curenv->SecondList), exist);
////			check_frame->in_second = 0;
////			pt_set_page_permissions(curenv->env_page_directory, exist->virtual_address, PERM_PRESENT, 0);
////			if (LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize) {
////				LIST_INSERT_HEAD(&(curenv->ActiveList), exist);
////				check_frame->in_active = 1;
////			}
////
////			else {
////				struct WorkingSetElement *vectim = LIST_LAST(&(curenv->ActiveList));
////				uint32*page_table_vectim = NULL;
////				get_page_table(curenv->env_page_directory, vectim->virtual_address, &page_table_vectim);
////				struct FrameInfo*vectim_frame = get_frame_info(curenv->env_page_directory, vectim->virtual_address, &page_table_vectim);
////				vectim_frame->in_active = 0;
////				vectim_frame->in_second = 1;
////				LIST_REMOVE(&(curenv->ActiveList), vectim);
////				pt_set_page_permissions(curenv->env_page_directory, vectim->virtual_address, 0, PERM_PRESENT);
////				LIST_INSERT_HEAD(&(curenv->SecondList), vectim);
////				LIST_INSERT_HEAD(&(curenv->ActiveList), exist);
////				check_frame->in_active = 1;
////			}
////		} else {
////			struct FrameInfo* new_frame;
////			int ret = allocate_frame(&new_frame);
////			if (ret == 0) {
////				ret = map_frame(curenv->env_page_directory, new_frame, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);
////				if (ret == 0) {
////
////					int ret3 = pf_read_env_page(curenv, (void*) fault_va);
////					//the page not exist in page file
////					if (ret3 == E_PAGE_NOT_EXIST_IN_PF) {
////						if ((fault_va < USER_HEAP_START || fault_va > USER_HEAP_MAX) && (fault_va > USTACKTOP || fault_va < USTACKBOTTOM)) {
////							unmap_frame(curenv->env_page_directory, fault_va);
////							//cprintf("3333333333333=%x\n",fault_va);
////							sched_kill_env(curenv->env_id);
////						}
//////					else
//////						ret=pf_update_env_page(curenv,fault_va,new_frame);
////						//	WE WILL UPDATE THE PAGE FILE TO CONTAIN THE FAULTED PAGE
////
////					}
////					if (ret == 0) {
////
////						struct WorkingSetElement*new_elm = env_page_ws_list_create_element(curenv, fault_va);
////						if (LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList)) < curenv->ActiveListSize + curenv->SecondListSize) {
////
////							//THRE IS STILL A PLACE IN THE MEMORY SO WE WILL DO A LRU PLACMENT
////							if (LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize) {
////								//pt_set_page_permissions(curenv->env_page_directory,new_elm->virtual_address,PERM_PRESENT,0);
////
////								LIST_INSERT_HEAD(&(curenv->ActiveList), new_elm);
////								new_frame->in_active = 1;
////							} else {
////
////								struct WorkingSetElement *vectim = LIST_LAST(&(curenv->ActiveList));
////								uint32*page_table_vectim = NULL;
////								get_page_table(curenv->env_page_directory, vectim->virtual_address, &page_table_vectim);
////								struct FrameInfo*vectim_frame = get_frame_info(curenv->env_page_directory, vectim->virtual_address, &page_table_vectim);
////								vectim_frame->in_active = 0;
////								vectim_frame->in_second = 1;
////								LIST_REMOVE(&(curenv->ActiveList), vectim);
////								pt_set_page_permissions(curenv->env_page_directory, vectim->virtual_address, 0, PERM_PRESENT);
////								LIST_INSERT_HEAD(&(curenv->SecondList), vectim);
////								LIST_INSERT_HEAD(&(curenv->ActiveList), new_elm);
////								new_frame->in_active = 1;
////							}
////						} else {
////
////							//THRE IS NO PLACE IN THE MEMORY SO WE WILL DO A LRU REPLACMENT
////							struct WorkingSetElement *dead = LIST_LAST(&(curenv->SecondList));
////							if ((pt_get_page_permissions(curenv->env_page_directory, dead->virtual_address) & PERM_MODIFIED) == PERM_MODIFIED) {
////
////								uint32*dead_page_table = NULL;
////								get_page_table(curenv->env_page_directory, dead->virtual_address, &dead_page_table);
////								struct FrameInfo*dead_frame = get_frame_info(curenv->env_page_directory, dead->virtual_address, &dead_page_table);
////								pf_update_env_page(curenv, dead->virtual_address, dead_frame);
////
////							}
////
////							env_page_ws_invalidate(curenv, dead->virtual_address);
////
////							//unmap_frame(curenv->env_page_directory,dead->virtual_address);
//////											LIST_REMOVE(&(curenv->SecondList),dead);
////							//working_set_elm_free(dead);
////							struct WorkingSetElement *vectim = LIST_LAST(&(curenv->ActiveList));
////							uint32*page_table_vectim = NULL;
////							get_page_table(curenv->env_page_directory, vectim->virtual_address, &page_table_vectim);
////							struct FrameInfo *vectim_frame = get_frame_info(curenv->env_page_directory, vectim->virtual_address, &page_table_vectim);
////							vectim_frame->in_active = 0;
////							vectim_frame->in_second = 1;
////							LIST_REMOVE(&(curenv->ActiveList), vectim);
////							pt_set_page_permissions(curenv->env_page_directory, vectim->virtual_address, 0, PERM_PRESENT);
////							LIST_INSERT_HEAD(&(curenv->SecondList), vectim);
////
////							LIST_INSERT_HEAD(&(curenv->ActiveList), new_elm);
////							new_frame->in_active = 1;
////
////						}
////					}
////
////				}
////
////			}
////
////		}
////
////	}
//
//}
//void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va) {
//	panic("this function is not required...!!");
//}



