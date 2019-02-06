#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*	ldelete() - Delete a given lock		*/

int ldelete(int des){
	STATWORD procstat;
	disable(procstat);
	if(isbadlock(des) || locks[des].lstate == LFREE){
		restore(procstat);
		return SYSERR;
	}
	locks[des].lstate = LFREE;
	int pid;
	int q = locks[des].qhead;
	while(q!= -1){
		pid = q;
		proctab[pid].pwaitret = DELETED;
		ready(pid, RESCHNO);
		q = lqueue[q].next;	
	}
	int i = 0;
	while(i < NPROC){
		if(locks[des].heldby[i] == 1){
		locks[des].heldby[i] = 0;
		setnewpinh(i);
		}
		i++;
	}
	locks[des].ltype = NOTSET;
	locks[des].lock_cnt = 0;
	locks[des].currproc = NULL;
	locks[des].qhead = -1;
	resched();//?
	restore(procstat);
	return OK;
}
