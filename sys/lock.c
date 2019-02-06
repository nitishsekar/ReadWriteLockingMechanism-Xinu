#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>
extern unsigned long ctr1000;

void setnewpinh(int);
void lenqueue(int, int, int, int);
void updateheldprio(int);

/*	lock() - get lock	*/

int lock(int des, int type, int priority){
	//kprintf("DEBUG: lock() called\n");
	if((type != READ && type != WRITE) || des < 0 || des >= NLOCKS){
		//kprintf("DEBUG: Error in lock()\n\tType: %d\n\tLock: %d\n", type, des);
		return SYSERR;
	}
	STATWORD procstat;
	disable(procstat);	
	//kprintf("DEBUG: Lock %d state is %d\n", des, locks[des].lstate);
	if(locks[des].ltype == NOTSET){
		//kprintf("DEBUG: Lock %d is free\n");
		locks[des].lstate = LACQ;
		locks[des].ltype = type;
		locks[des].lock_cnt++;
		if(locks[des].currproc == NULL)
			locks[des].currproc = currpid;//?
		
		struct pentry *pptr = &proctab[currpid];
		
		if(pptr->lockby[des] == -1){
			//kprintf("DEBUG: Storing lockby\n");
			pptr->lockby[des] = locks[des].currproc;
		}
		else{
			if(pptr->lockby[des] != locks[des].currproc){
				restore(procstat);
				//kprintf("DEBUG: Error. Can't access other proc's lock\n");
				return SYSERR;
			}
		}
		locks[des].heldby[currpid] = 1;
		pptr->lockheld[des] = 1;
		restore(procstat);
		return OK;
	}
	else{
		//kprintf("DEBUG: Lock %d not free\n", des);
		struct pentry *pptr = &proctab[currpid];
		if(pptr->lockby[des] == -1){
			//kprintf("DEBUG: Storing lockby\n");
			pptr->lockby[des] = locks[des].currproc;
		}
		else{
			if(pptr->lockby[des] != locks[des].currproc){
				restore(procstat);
				//kprintf("DEBUG: Error. Can't access other proc's lock\n");
				return SYSERR;
			}
		}
		if(locks[des].ltype == READ){
			//kprintf("DEBUG: Lock type is READ\n");
			if(type == READ){
				//kprintf("DEBUG: Requested type is READ\n");
				int q = locks[des].qhead;
				while((q!= -1) && (lqueue[q].type != WRITE)){
					q = lqueue[q].next;
				}
				if(q == -1 || lqueue[q].prio <= priority){
					locks[des].lock_cnt++;
					//kprintf("DEBUG: New lock count for lock %d is %d\n", des, locks[des].lock_cnt);
					pptr->lockheld[des] = 1;
					restore(procstat);
					return OK;
				}
			}
			int pr = proctab[currpid].pinh;
			if(pr==0)
				pr = getprio(currpid);
			if(pr > locks[des].maxprio){
				locks[des].maxprio = pr;
			}
			//kprintf("DEBUG: Adding process %s to queue for lock %d\n", proctab[currpid].pname, des);
			lenqueue(currpid, priority, type, des);
			updateheldprio(des);
			pptr->pstate = PRWAIT;
			pptr->pwaitret = OK;
			pptr->lock_wait = des;
			resched();
			pptr->lock_wait = -1;
			if(pptr->pwaitret == OK){
				pptr->lockheld[des] = 1;
				updateheldprio(des);
			}
			restore(procstat);
			return pptr->pwaitret;
			
		}
		if(locks[des].ltype == WRITE){
			//kprintf("DEBUG: Lock type is WRITE for %d\n", des);
			if(priority > locks[des].maxprio){
				locks[des].maxprio = priority;
			}
			//kprintf("DEBUG: Adding process %s to queue for lock %d\n", proctab[currpid].pname, des);
			lenqueue(currpid, priority, type, des);
			updateheldprio(des);
			pptr->pstate = PRWAIT;
			pptr->pwaitret = OK;
			pptr->lock_wait = des;
			resched();
			pptr->lock_wait = -1;
			if(pptr->pwaitret == OK){
				pptr->lockheld[des] = 1;
				updateheldprio(des);
			}
			restore(procstat);
			return pptr->pwaitret;
		}
	}
}

void lenqueue(int pid, int prio, int type, int des){
	struct lproc_q *q = &lqueue[pid];
		
		q->prio = prio;
		q->type = type;
		q->wait_time = ctr1000;
		q->next = -1;
	//kprintf("DEBUG: New queue entry\n\tPID: %d\n\tPrio: %d\n\tType: %d\n\tWait time: %x\n",pid, q->prio, q->type, q->wait_time);
	int head = locks[des].qhead;
	//kprintf("DEBUG: q: 0x%08x\n", &q);
	if(head == -1){
	
		//locks[des].q_cnt++;
		locks[des].qhead = pid;
		int t = locks[des].qhead;
		//kprintf("DEBUG: Added queue entry\n\tPID: %d\n\tPrio: %d\n\tType: %d\n\tWait time: %x\n",t, lqueue[t].prio, lqueue[t].type, lqueue[t].wait_time);
		//kprintf("DEBUG: Printing queue\n");
		/*t = locks[des].qhead;
		while(t != -1){
		kprintf("DEBUG: Queue entry\n\tPID: %d\n\tPrio: %d\n\tType: %d\n\tWait time: %x\n",t, lqueue[t].prio, lqueue[t].type, lqueue[t].wait_time);
		t = lqueue[t].next;
		}*/
		int pr = getprio(pid);
		if(pr > locks[des].maxprio)
			locks[des].maxprio = pr;
	}
	else{
		
		int t = locks[des].qhead;
		//kprintf("DEBUG: Existing queue\n");
		/*while(t != -1){
		kprintf("DEBUG: Queue entry\n\tPID: %d\n\tPrio: %d\n\tType: %d\n\tWait time: %x\n",t, lqueue[t].prio, lqueue[t].type, lqueue[t].wait_time);
		t = lqueue[t].next;
		}*/
		int prev = locks[des].qhead;
		head = lqueue[head].next;
		while(head != -1 || lqueue[head].prio >= prio){
			
			prev = head;
			head = lqueue[head].next;
		}
		lqueue[prev].next = pid;
		q->next = head;
		/*kprintf("DEBUG: Printing queue\n");
		t = locks[des].qhead;
		while(t != -1){
		kprintf("DEBUG: Queue entry\n\tPID: %d\n\tPrio: %d\n\tType: %d\n\tWait time: %x\n",t, lqueue[t].prio, lqueue[t].type, lqueue[t].wait_time);
		t = lqueue[t].next;
		}*/
		int pr = getprio(pid);
		if(pr > locks[des].maxprio)
			locks[des].maxprio = pr;
	}
}

void updateheldprio(int des){
	//kprintf("DEBUG: Updating prio (inheritance)\n");
	int i = 0;
	while(i < NPROC){
		if(locks[des].heldby[i] == 1){
			//kprintf("DEBUG: Lock %d held by %d\n", des, i);
			/*int pr = proctab[i].pinh;
			if(pr == 0)*/
			int pr = getprio(i);
			if(pr < locks[des].maxprio)
				chprioinh(i,locks[des].maxprio);
			else
				setnewpinh(i);
		}
		i++;
	}
}

void chprioinh(int pid, int prio){
	proctab[pid].pinh = prio;
	if(proctab[pid].lock_wait != -1){
		int i = 0;
		while(i<NPROC){
			if(locks[proctab[pid].lock_wait].heldby[i] == 1){
				/*int pr = proctab[i].pinh;
				if(pr == 0)*/
				int pr = getprio(i);
				if(pr < prio)
					chprioinh(i, prio);
			}
			i++;
		}
	}
}

void setnewpinh(int pid){
	struct pentry *pptr = &proctab[pid];
	int i = 0, max = 0;
	while(i < NLOCKS){
		if(pptr->lockheld[i] == 1){
			if(locks[i].maxprio > max)
				max = locks[i].maxprio;
		}
		i++;
	}
	if(max > pptr->pprio)
		pptr->pinh = max;
	else
		pptr->pinh = 0;
}
