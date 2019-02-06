/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);
	int i = 0;
	while(i<NLOCKS){
		if(pptr->lockheld[i] == 1)
			releaseall(1,i);
		i++;
	}	
	if(pptr->lock_wait != -1){
		int q = locks[pptr->lock_wait].qhead;
		int p;
		//p->next = q;
		//q = q->next;
		if(lqueue[q].next== -1){
			locks[pptr->lock_wait].qhead = lqueue[locks[pptr->lock_wait].qhead].next;
			locks[pptr->lock_wait].maxprio = -1;
			int i = 0;
			while(i < NPROC){
				if(locks[pptr->lock_wait].heldby[i] == 1)
					setnewpinh(i);
				i++;
			}
		}
		else{
		p = q;
		q = lqueue[q].next;
		int flag = 0;
		if(p == pid)
			locks[pptr->lock_wait].qhead = q;
		else{
		while(flag == 0){
			if(q == pid){
				lqueue[p].next = lqueue[q].next;
				flag = 1;
			}
			p = q;
			q = lqueue[q].next;
		}
		}
		q = locks[pptr->lock_wait].qhead;
		int max = 0;
		while(q!= -1){
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
		}
		pptr->lock_wait = -1;
	}

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
