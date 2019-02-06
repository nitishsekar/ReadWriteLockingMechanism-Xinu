/*	lock.h		*/
#ifndef _LOCK_H_
#define _LOCK_H_

#define NLOCKS 50

#define LFREE		1
#define LACQ		2
#define READ		3
#define WRITE		4
#define NOTSET		5
#define LDELETED	6

#define isbadlock(l)	(l<0 || l>=NLOCKS)

struct lproc_q {
	//int	procid;
	int	prio;
	int	type;
	unsigned long wait_time; // Store ctr1000 at wait time
	int 	next;
};

struct lentry {
	int	lstate;
	int	ltype;
	int	lock_cnt;
	int	currproc;
	int	heldby[NPROC];
	int 	maxprio;
	//int	q_cnt;
	int 	qhead;
};
extern struct lproc_q lqueue[];
extern struct lentry locks[];
//extern void linit();
int lcreate();
int lock(int, int, int);
SYSCALL releaseall(int, long, ...);
extern void setnewpinh(int);
extern void lenqueue(int, int, int, int);
extern void updateheldprio(int);
#endif
