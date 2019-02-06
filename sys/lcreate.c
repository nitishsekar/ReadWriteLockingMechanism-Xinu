#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

LOCAL int newlock();

/*	lcreate() - Create a new lock		*/

int lcreate(){
	STATWORD procstat;
	int l;
	disable(procstat);
	if((l=newlock()) == SYSERR){
		restore(procstat);
		return SYSERR;
	}

	locks[l].lstate = LACQ;
	locks[l].currproc = currpid;
	restore(procstat);
	return l;
}

/*	newlock() - Get new lock ID	*/

LOCAL int newlock(){
	int i = 0;
	while(i<NLOCKS){
		if(locks[i].lstate == LFREE){
			return i;
		}
		i++;
	}
	return SYSERR;
}
