// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_semaphore is not implemented yet");
	//Your Code is Here...

	struct semaphore new_semaphore;

	struct __semdata* semaphore_data = smalloc(semaphoreName, sizeof(struct __semdata), 1);

	if (semaphore_data == NULL)
	{
		panic("failed to create semaphore");
	}

	semaphore_data->count = value;
	semaphore_data->lock = 0;
	strcpy(semaphore_data->name, semaphoreName);
	LIST_INIT(&semaphore_data->queue);

	new_semaphore.semdata = semaphore_data;

//	cprintf("semaphore created successfully\n");
	return new_semaphore;
}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_semaphore is not implemented yet");
	//Your Code is Here...

	struct semaphore required_semaphore;

	struct Share* semaphore_data = (struct Share*) sget(ownerEnvID, semaphoreName);

	if (semaphore_data == NULL)
	{
		panic("semaphore is not existed\n");
	}

	struct __semdata* sem_data = (struct __semdata*) semaphore_data;

	required_semaphore.semdata = sem_data;

//	cprintf("semaphore found successfully\n");
	return required_semaphore;
}

void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("wait_semaphore is not implemented yet");
	//Your Code is Here...

	// acquire
	uint32 key = 1;
	do{ xchg(&key, sem.semdata->lock); } while(key != 0);

	sem.semdata->count--;

	if (sem.semdata->count < 0)
	{
//		cprintf("sleeping\n");
		// insert in the waiting queue
//		cprintf("before adding in %s the size was %d\n", sem.semdata->name, LIST_SIZE(&sem.semdata->queue));
		sys_insert_env_in_waiting_queue(&sem.semdata->queue);
//		cprintf("after adding in %s the size was %d\n", sem.semdata->name, LIST_SIZE(&sem.semdata->queue));

		// realse before blocking
		sem.semdata->lock = 0;

		// block the curr env
		sys_block_curr_env();
//		cprintf("blocked\n");
	}

	// release
	sem.semdata->lock = 0;
}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("signal_semaphore is not implemented yet");
	//Your Code is Here...

	// acquire
	uint32 key = 1;
	do{ xchg(&key, sem.semdata->lock); } while(key != 0);

	sem.semdata->count++;

	if (sem.semdata->count <= 0)
	{
//		cprintf("before removing in %s the size was %d\n", sem.semdata->name, LIST_SIZE(&sem.semdata->queue));
		struct Env* first_in_q_env = (struct Env*) sys_remove_env_from_waiting_queue(&sem.semdata->queue);
//		cprintf("after removing in %s the size was %d\n", sem.semdata->name, LIST_SIZE(&sem.semdata->queue));

		// set that env status to ready and insert it in the ready queue
//		cprintf("waiking up the sleeped env\n");
		sys_insert_env_in_ready_queue(first_in_q_env);
	}

//	cprintf("finished signaling\n");
	//release
	sem.semdata->lock = 0;
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
