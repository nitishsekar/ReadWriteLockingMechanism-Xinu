/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	pptr->pprio = newprio;
	if(newprio > pptr->pinh)
		pptr->pinh = 0;
	if(pptr->lock_wait != -1){
			
		int q = locks[pptr->lock_wait].qhead;
		int max = 0;
		while(q!=-1){
			int pr = proctab[q].pinh;
			if(pr == 0)
				pr = getprio(q);
			if(pr > max)
				max = pr;
			q = lqueue[q].next;
		}
		locks[pptr->lock_wait].maxprio = max;
		int i = 0;
		while(i<NPROC){
			if(locks[pptr->lock_wait].heldby[i]==1)
				setnewpinh(i);
			i++;
		}
		
		pptr->lock_wait = -1;
	}
	restore(ps);
	return(newprio);
}
