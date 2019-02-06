#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

/*	releaseall()	*/
SYSCALL releaseall(int num, long args, ...){
	STATWORD procstat;
	disable(procstat);
	int response = OK;
	int i = 0;
	while(i < num){
		response = releaselock((int)*((&args) + i));
		i++;
	}
	resched();
	restore(procstat);
	return response;
}

/*	releaselock()	*/

SYSCALL releaselock(int des){
	if(des < 0 || des >= NLOCKS || locks[des].ltype == NOTSET)
		return SYSERR;
	//kprintf("DEBUG: Releasing lock %d\n", des);
	STATWORD procstat;
	disable(procstat);
	locks[des].lock_cnt--;
	locks[des].heldby[currpid] = 0;
	proctab[currpid].lockheld[des] = 0;
	setnewpinh(currpid);
	if(locks[des].lock_cnt == 0){
		//kprintf("DEBUG: Lock count is zero for lock %d\n", des);
		int h = locks[des].qhead;
		//kprintf("DEBUG: queue entry\n\tPID: %d\n\tPrio: %d\n\tType: %d\n\tWait time: %x\n",h, lqueue[h].prio, lqueue[h].type, lqueue[h].wait_time);
		if(h == -1){
			//kprintf("DEBUG: Queue is empty for lock %d\n", des);
			//ldelete(des);
			locks[des].lstate = LFREE;
			locks[des].ltype = NOTSET;
			locks[des].maxprio = -1;
			restore(procstat);
			return OK;	
		}
		else{
			int n = lqueue[h].next;
			if(n == -1){
				//kprintf("DEBUG: One process in queue\n");
				locks[des].ltype = lqueue[h].type;
				//kprintf("DEBUG: Setting lock type to %d\n", lqueue[h].type);
				locks[des].lock_cnt++;
				locks[des].heldby[h] = 1;
				proctab[h].lockheld[des] = 1;
				if(proctab[h].lockby[des] == -1)
					proctab[h].lockby[des] = locks[des].currproc;
				locks[des].maxprio = -1;
				//kprintf("DEBUG: Process added to ready queue\n");
				ready(h,RESCHNO);
				locks[des].qhead = lqueue[locks[des].qhead].next;
				updateheldprio(des);
				restore(procstat);
				return OK;
			}
			else{
				if(lqueue[h].prio > lqueue[n].prio){
					locks[des].ltype = lqueue[h].type;
					proctab[h].lockheld[des] = 1;
					locks[des].heldby[h] = 1;
					if(proctab[h].lockby[des] == NULL)
						proctab[h].lockby[des] = locks[des].currproc;
					locks[des].lock_cnt++;
					ready(h,RESCHNO);
					locks[des].qhead = lqueue[locks[des].qhead].next;
					if(lqueue[h].type == READ){
						h = lqueue[h].next;
						while(h != -1 || lqueue[h].type != WRITE){
							locks[des].lock_cnt++;
							proctab[h].lockheld[des] = 1;
							locks[des].heldby[h] = 1;
							if(proctab[h].lockby[des] == NULL)
								proctab[h].lockby[des] = locks[des].currproc;
							ready(h,RESCHNO);
							locks[des].qhead = lqueue[locks[des].qhead].next;
						}
					}
					int q = locks[des].qhead;
					int max = 0;
					while(q!=-1){
						/*int pr = proctab[q].pinh;
						if(pr == 0)*/
						int pr = getprio(q);
						if(pr > max)
							max = pr;
						q = lqueue[q].next;
					}
					locks[des].maxprio = max;
					updateheldprio(des);
					restore(procstat);
					return OK;
				}
				else{
					if(lqueue[h].type == WRITE){
						locks[des].ltype = h;
						locks[des].lock_cnt++;
						locks[des].heldby[h] = 1;
						proctab[h].lockheld[des] = 1;
						if(proctab[h].lockby[des] == NULL)
							proctab[h].lockby[des] = locks[des].currproc;
						ready(h,RESCHNO);
						locks[des].qhead = lqueue[locks[des].qhead].next;
						int q = locks[des].qhead;
						int max = 0;
						while(q!= -1){
							/*int pr = proctab[q].pinh;
							if(pr == 0)*/
							int pr = getprio(q);
							if(pr > max)
								max = pr;
							q = lqueue[q].next;
						}
						locks[des].maxprio = max;
						updateheldprio(des);
						restore(procstat);
						return OK;
					}
					else{
						if(lqueue[n].type == WRITE && (lqueue[n].wait_time - lqueue[h].wait_time) < 1000){
							locks[des].ltype = lqueue[n].type;
							locks[des].lock_cnt++;
							locks[des].heldby[n] = 1;
							proctab[n].lockheld[des] = 1;
							if(proctab[n].lockby[des] == NULL)
								proctab[n].lockby[des] = locks[des].currproc;
							lqueue[h].next = lqueue[n].next;
							ready(n,RESCHNO);
							int q = locks[des].qhead;
							int max = 0;
							while(q!= -1){
								int pr = proctab[q].pinh;
								if(pr == 0)
									pr = getprio(q);
								if(pr > max)
									max = pr;
								q = lqueue[q].next;
							}
							locks[des].maxprio = max;
							updateheldprio(des);
							restore(procstat);
							return OK;
						}
						else{
							locks[des].ltype = lqueue[h].type;
							locks[des].lock_cnt++;
							locks[des].heldby[h] = 1;
							proctab[h].lockheld[des] = 1;
							if(proctab[h].lockby[des] == NULL)
								proctab[h].lockby[des] = locks[des].currproc;
							ready(h,RESCHNO);
							locks[des].qhead = lqueue[locks[des].qhead].next;
							h = lqueue[h].next;
							while(h != -1 || lqueue[h].type != WRITE){
								locks[des].lock_cnt++;
								locks[des].heldby[h] = 1;
								proctab[h].lockheld[des] = 1;
								if(proctab[h].lockby[des] == NULL)
									proctab[h].lockby[des] = locks[des].currproc;
								ready(h,RESCHNO);
								locks[des].qhead = lqueue[locks[des].qhead].next;
							}
							int q = locks[des].qhead;
							int max = 0;
							while(q!= -1){
								int pr = proctab[q].pinh;
								if(pr == 0)
									pr = getprio(q);
								if(pr > max)
									max = pr;
								q = lqueue[q].next;
							}
							locks[des].maxprio = max;
							updateheldprio(des);
							restore(procstat);
							return OK;
						}
					}
				}
			}
		}
	}
	else{
		restore(procstat);
		return OK;
	}
}
