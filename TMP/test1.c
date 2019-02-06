/*	Pseudo Code	*/

struct test_lock {
	int type;
	int state;
	int lockprio; // Set to a default value during initialization
	int qhead;
	unsigned long wait;
};

struct test_lock tlocks[NLOCKS];

void test_linit(){
	int i = 0;
	while(i<NLOCKS){
	// Init other fields
		tlocks[i].lockprio = 45; // Setting 45 as the default lock priority
		i++;
	}
	
}

int test_lcreate(int prio){
	int l;
	// Same implementation as lcreate(), with one additional line
	if(prio>tlocks[l].lockprio)
		tlocks[l].lockprio = prio; // Creating process can increase this priority during creation
	return l;
}

// Process
void test_proc(int l){
	lock(l, READ); // Process acquiring the lock will have its priority changed to lockprio (50 in this case)
	// Rest of the program...
}

void main(){
	int lock = lcreate(50); // Setting the lock priority to 50
	// Create process pid1 with priority 25, and pid2 with riority 40.
	resume(pid1); // pid1 acquires lock. Has prioty changed to 45.
	resume(pid2); // pid2 waits on the lock. 
	// Now, even though originally pid1 has a lesser priority than pid2, priority inversion is avoided as pid1 has its priority changed to 50.
}
