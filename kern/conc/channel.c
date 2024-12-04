/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>

//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan, struct spinlock* guard) {
    //TODO: [PROJECT'24.MS1 - #10] [4] LOCKS - sleep
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("sleep is not implemented yet");
    //Your Code is Here...
    //cprintf("Channel name : %s\n", chan->name);
    acquire_spinlock(&ProcessQueues.qlock);

    struct Env *env = get_cpu_proc();

    //
    env->env_status = ENV_BLOCKED;
    enqueue(&(chan->queue), env);
    release_spinlock(guard); // release guard to enable other process (that will wake up) to re-check the sleep-lock
    //

    sched(); // call another ready process

    //cprintf("I woke up!");
    //

    release_spinlock(&ProcessQueues.qlock); // to enable wake-ups
    acquire_spinlock(guard); // reaching this line means the current process got waken up
    //

    //struct Env *env2 = get_cpu_proc();
    //assert(env2 != env);

}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan) {
    acquire_spinlock(&ProcessQueues.qlock); // to avoid any others getting waken up at the same time
    struct Env *env = dequeue(&(chan->queue));
    if (env != NULL) {
        // Set the environment status to ready so it can be scheduled
        assert(env->env_status = ENV_BLOCKED);
        env->env_status = ENV_READY;
        sched_insert_ready0(env);
        //cprintf("env: %c", env->prog_name);
    }
    // Another idea: loop over the queue, if the process is BLOCKED, make it READY.
    /*
     * what it does is just not take input then `nothing`, it just is sleeping I guess with no
     */
    release_spinlock(&ProcessQueues.qlock);
}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan) {
    if (holding_spinlock(&ProcessQueues.qlock)) {
        panic("wtf??");
    }
    acquire_spinlock(&ProcessQueues.qlock); // to avoid any others getting waken up at the same time
    struct Env *env = dequeue(&(chan->queue));
    //cprintf("%d", env == NULL);
    //uint32 cnt = 0;

    while (env != NULL) {
        //cnt++;
        //cprintf("cnt : %u", cnt);
        assert(env->env_status = ENV_BLOCKED);
        env->env_status = ENV_READY;
        sched_insert_ready0(env);
        env = dequeue(&(chan->queue));
    //sched();
    }

    //cprintf("Hey, I'm here!");
    release_spinlock(&ProcessQueues.qlock);
}
