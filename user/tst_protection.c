#include <inc/lib.h>

void
_main(void)
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN KERNEL DYNAMIC ALLOCATOR DUE
	 * TO sbrk()
	 *********************************************************/

	/*=================================================*/
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
#if USE_KHEAP
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif
	/*=================================================*/

	/*char initname[10] = "x";
	char name[10] ;
#define NUM_OF_OBJS 5000
	uint32* vars[NUM_OF_OBJS];
	for (int s = 0; s < NUM_OF_OBJS; ++s)
	{
		char index[10];
		ltostr(s, index);
		strcconcat(initname, index, name);
		vars[s] = smalloc(name, PAGE_SIZE, 1);
		*vars[s] = s;
	}*/
	int numOfSlaves = 3;
	rsttst();
	//[1] Run programs that allocate many shared variables
	for (int i = 0; i < numOfSlaves; ++i) {
		int32 envId = sys_create_env("protection_slave1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
		sys_run_env(envId);
		//cprintf("\n Slave %d done!, tst: %u \n", i, gettst());
	}

	while (gettst() != numOfSlaves)
		/*cprintf("\n gettst(): %u \n", gettst())*/;

	cprintf("%~\nCongratulations... test protection is run successfully\n");

}
