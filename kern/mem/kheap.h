#ifndef FOS_KERN_KHEAP_H_
#define FOS_KERN_KHEAP_H_

#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>


/*2017*/
uint32 _KHeapPlacementStrategy;
//Values for user heap placement strategy
#define KHP_PLACE_CONTALLOC 0x0
#define KHP_PLACE_FIRSTFIT 	0x1
#define KHP_PLACE_BESTFIT 	0x2
#define KHP_PLACE_NEXTFIT 	0x3
#define KHP_PLACE_WORSTFIT 	0x4

static inline void setKHeapPlacementStrategyCONTALLOC(){_KHeapPlacementStrategy = KHP_PLACE_CONTALLOC;}
static inline void setKHeapPlacementStrategyFIRSTFIT(){_KHeapPlacementStrategy = KHP_PLACE_FIRSTFIT;}
static inline void setKHeapPlacementStrategyBESTFIT(){_KHeapPlacementStrategy = KHP_PLACE_BESTFIT;}
static inline void setKHeapPlacementStrategyNEXTFIT(){_KHeapPlacementStrategy = KHP_PLACE_NEXTFIT;}
static inline void setKHeapPlacementStrategyWORSTFIT(){_KHeapPlacementStrategy = KHP_PLACE_WORSTFIT;}

static inline uint8 isKHeapPlacementStrategyCONTALLOC(){if(_KHeapPlacementStrategy == KHP_PLACE_CONTALLOC) return 1; return 0;}
static inline uint8 isKHeapPlacementStrategyFIRSTFIT(){if(_KHeapPlacementStrategy == KHP_PLACE_FIRSTFIT) return 1; return 0;}
static inline uint8 isKHeapPlacementStrategyBESTFIT(){if(_KHeapPlacementStrategy == KHP_PLACE_BESTFIT) return 1; return 0;}
static inline uint8 isKHeapPlacementStrategyNEXTFIT(){if(_KHeapPlacementStrategy == KHP_PLACE_NEXTFIT) return 1; return 0;}
static inline uint8 isKHeapPlacementStrategyWORSTFIT(){if(_KHeapPlacementStrategy == KHP_PLACE_WORSTFIT) return 1; return 0;}

//***********************************

void* HARD_LIMIT;
void* PAGE_ALLOCATION_START;
void* BLOCK_ALLOCATION_START;
void* BREAK;

void* kmalloc(unsigned int size);
void kfree(void* virtual_address);
void *krealloc(void *virtual_address, unsigned int new_size);

unsigned int kheap_virtual_address(unsigned int physical_address);
unsigned int kheap_physical_address(unsigned int virtual_address);

int numOfKheapVACalls ;

struct spinlock umlk;

//uint32 pc[1 << 20];
//
////************************************
//// Dynamic segment tree :
//// To be initialized in kheap initializer
//
//typedef struct Node {
//    uint32 address;    // adderss of mx_size node, also guide when adding
//    uint32 size;        // The guide when traversing
//    uint32 l;
//    uint32 r;
//} Node;
//
//Node slot[900000];
//
//Node merge(Node parent, Node left, Node right);
//
//uint32 node_counter; // 1 for root
//
///*
// * Adds a slot with a specific address and size keeping the slots ordered by address
// */
//void add(uint32 address, uint32 size);
//void add1(uint32 ti, uint32 tl, uint32 tr, uint32 address, uint32 size);
//
//
///*
// * remove node connected to address
// */
//void remove(uint32 address);
//void remove1(uint32 ti, uint32 tl, uint32 tr, uint32 address);
//
//
///*
// * get the node_info connected to address
// */
//Node get(uint32 address);
//Node get1(uint32 ti, uint32 tl, uint32 tr, uint32 address);
//
///*
// * Behaves the same as get_first_bigger except when it gets to the required
// * node, it erases it and remerges the ancestor nodes
// */
//void remove_first_bigger(uint32 req_size);
//void remove_first_bigger1(uint32 ti, uint32 tl, uint32 tr, uint32 req_size);
//
///*
// *  Returns address of first node with size >= req_size in ~O(log(n)), `n` being the node count
// */
//Node get_first_bigger(uint32 req_size);
//Node get_first_bigger1(uint32 ti, uint32 tl, uint32 tr, uint32 req_size);


#endif // FOS_KERN_KHEAP_H_
