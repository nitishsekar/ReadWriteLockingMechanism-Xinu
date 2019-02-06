#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*	linit() - Initialize all locks	*/

void linit(){
	int i = 0;
	while(i < NLOCKS){
		locks[i].lstate = LFREE;
		locks[i].ltype = NOTSET;
		locks[i].lock_cnt = 0;
		locks[i].currproc = NULL;
		locks[i].maxprio = 0;
		//locks[i].q_cnt = 0;
		int j=0;
		while(j<NPROC){
			locks[i].heldby[j] = 0;
			j++;
		}
		locks[i].qhead = -1;
		i++;
	}
	i = 0;
	while(i < NPROC){
		lqueue[i].prio = -1;
		lqueue[i].type = -1;
		lqueue[i].wait_time = 0;
		lqueue[i].next = -1;
		i++;
	}
	//kprintf("DEBUG: Locks initialized\n");
}
