#include "scheduling_simulator.h"

tasklist_t head;
tasklist_t *newtask;
tasklist_t *tail = &head;
tasklist_t *nowtask = &head;
tasklist_t *nowfront = &head;
ucontext_t Shell;
ucontext_t Simulator;
ucontext_t Finish;
struct itimerval timerrr;
int countask = 0;

void set_timer(int sec, int usec)
{
	timerrr.it_value.tv_sec = sec;
	timerrr.it_value.tv_usec = usec;
	setitimer(ITIMER_REAL, &timerrr, NULL);
	return;
}

void hw_suspend(int msec_10)
{
	// puts("suspend.");
	tasklist_t *ptr = nowtask;
	tasklist_t *now = nowtask;
	set_timer(0,0);
	nowtask->state = TASK_WAITING;
	nowtask->susptime = msec_10 * 10;
	while(ptr->next != NULL) {
		nowfront = ptr;
		ptr = ptr->next;
		if(ptr->state == TASK_READY) {  //find next ready task
			nowtask = ptr;  //next
			ptr->state = TASK_RUNNING;
			if(nowtask->timequ == 20) { //set time quantum
				set_timer(0,20000);
			} else {
				set_timer(0,10000);
			}
			// printf("switch from %d %s", now->pid, now->name);
			// printf(" to %d %s\n", nowtask->pid, nowtask->name);
			swapcontext(&(now->context), &(nowtask->context));
			return;
		}
	}
	ptr = &head;    //if no ready task next, search before
	while(ptr->next != NULL && ptr->next != nowtask) {
		nowfront = ptr;
		ptr = ptr->next;
		if(ptr->state == TASK_READY) {  //find next ready task
			nowtask = ptr;  //next
			ptr->state = TASK_RUNNING;
			if(nowtask->timequ == 20) {
				set_timer(0,20000);
			} else {
				set_timer(0,10000);
			}
			// printf("switch from %d %s", now->pid, now->name);
			// printf(" to %d %s\n", nowtask->pid, nowtask->name);
			swapcontext(&(now->context), &(nowtask->context));
			return;
		}
	}
	puts("all not ready.");	//if no ready task at all
	puts("waiting...");
	swapcontext(&(nowtask->context), &Simulator);   //now:WAITING
	return;
}

void hw_wakeup_pid(int pid)
{
	tasklist_t *ptr = &head;
	while(ptr->next != NULL) {
		ptr = ptr->next;
		if(ptr->pid == pid && ptr->state == TASK_WAITING) {
			printf("%d %s woke up.\n", ptr->pid, ptr->name);
			ptr->state = TASK_READY;
			ptr->susptime = 0;
		}
	}
	return;
}

int hw_wakeup_taskname(char *task_name)
{
	tasklist_t *ptr = &head;
	int count = 0;
	while(ptr->next != NULL) {
		ptr = ptr->next;
		if(strcmp(ptr->name, task_name) == 0 && ptr->state == TASK_WAITING) {
			printf("%d %s woke up.\n", ptr->pid, ptr->name);
			ptr->state = TASK_READY;
			ptr->susptime = 0;
			count++;
		}
	}
	return count;
}

int hw_task_create(char *task_name)
{
	void (*func)(void); //link function
	if(strcmp(task_name, "task1") == 0) {
		func = task1;
	} else if(strcmp(task_name, "task2") == 0) {
		func = task2;
	} else if(strcmp(task_name, "task3") == 0) {
		func = task3;
	} else if(strcmp(task_name, "task4") == 0) {
		func = task4;
	} else if(strcmp(task_name, "task5") == 0) {
		func = task5;
	} else if(strcmp(task_name, "task6") == 0) {
		func = task6;
	} else {
		return -1;
	}
	newtask = (struct tasklist_t *)malloc(sizeof(tasklist_t));
	countask++; //pid
	newtask->pid = countask;
	strcpy(newtask->name, task_name);
	newtask->timequ = 10;
	newtask->next = NULL;
	newtask->state = TASK_READY;
	newtask->func = func;
	newtask->quingtime = 0;
	newtask->susptime = 0;

	getcontext(&(newtask->context));
	newtask->context.uc_link = &Finish;	//if terminate before time quantum
	newtask->context.uc_stack.ss_sp = malloc(4096);
	newtask->context.uc_stack.ss_size = 4096;
	newtask->context.uc_stack.ss_flags = 0;
	makecontext((&newtask->context), (void *)newtask->func, 0);

	tasklist_t *ptr = &head;    //add to list tail
	while(ptr->next != NULL) {
		ptr = ptr->next;
	}
	ptr->next = newtask;
	tail = newtask;
	printf("create %d %s\n", tail->pid, tail->name);
	return newtask->pid;
}

void tictok(int signal)
{
	// puts("time up!");
	set_timer(0,0);
	// printf("nowtask : %d %s %d\n", nowtask->pid, nowtask->name, nowtask->state);
	tasklist_t *ptr = &head;

	while(ptr->next != NULL) {
		ptr = ptr->next;
		if(ptr->state == TASK_READY) {  //add queueing time
			ptr->quingtime += nowtask->timequ;
		}
		if(ptr->state == TASK_WAITING) {    //decrease suspend time
			ptr->susptime = ptr->susptime - nowtask->timequ;
			if(ptr->susptime <= 0) {
				printf("%d %s woke up.\n", ptr->pid, ptr->name);
				ptr->state = TASK_READY;
				ptr->susptime = 0;
			}
		}
	}
	if(nowtask->state == TASK_RUNNING) {    //switch task
		nowtask->state = TASK_READY;
		if(nowtask->next != NULL) { //if not tail put to tail
			nowfront->next = nowtask->next;
			tail->next = nowtask;
			tail = nowtask;
			nowtask->next = NULL;
			// printf("stop %d %s back to simulator\n", tail->pid, tail->name);
			swapcontext(&(nowtask->context), &Simulator);   //now:READY
			return;
		}
	}
	//if in waiting loop or just wake up
	if(nowtask->state == TASK_WAITING || nowtask->state == TASK_READY) {
		//printf("stop %d %s back to simulator\n", nowtask->pid, nowtask->name;
		setcontext(&Simulator);	//back to simulator
		return;
	}
}

void pause(int signal)
{
	puts("\npaused.");
	set_timer(0,0);

	if(nowtask->state == TASK_RUNNING) {	//pause now running task
		// printf("stop %d %s back to shell\n", nowtask->pid, nowtask->name);
		swapcontext(&(nowtask->context), &Shell);
		return;
	}
	//if(nowtask->state == TASK_WAITING || nowtask->state == TASK_READY)
	//if in waiting loop or just wake up, back to shell
	//printf("stop %d %s back to simulator\n", nowtask->pid, nowtask->name);
	setcontext(&Shell);
	return;
}

void finish()
{
	set_timer(0,0);
	nowtask->state = TASK_TERMINATED;
	// printf("%s terminated\n", nowtask->name);
	setcontext(&Simulator);
	return;
}

void shell()
{
	puts("[Shell Mode]");
	int pid = 0;
	int timequ = 0;
	char command[30] = {};
	char task_name[10] = {};
	char *catch = NULL;
	set_timer(0,0);
	while(1) {
		printf("$ ");
		fgets(command, 30, stdin);	//get command line
		catch = strtok(command, " \n");
		if(catch[0] == 'a') {   //add
				catch = strtok(NULL, " \n");
				strcpy(task_name, catch);
				catch = strtok(NULL, " \n");
				timequ = 10;
				if(catch != NULL) { //-t
						catch = strtok(NULL, " \n");
						if(catch[0] == 'L')
								timequ = 20;
						else
							timequ = 10;
					}
				pid = hw_task_create(task_name);
				if(pid == -1) {	//not found
					puts("task not exist.");

				} else {
					tail->timequ = timequ;  //new add task at tail
					printf("add %d %s %d\n", tail->pid, tail->name, tail->timequ);
				}
			}

			else if(catch[0] == 'r') {  //remove
					catch = strtok(NULL, " ");
					pid = atoi(catch);
					tasklist_t *ptr = &head;
					tasklist_t *prev = &head;
					if(ptr->next == NULL) {
						puts("queue is empty.");
					} else {
						ptr = ptr->next;
						while(ptr->pid != pid && ptr->next != NULL) {
							prev = ptr;
							ptr = ptr->next;
						}
						if(ptr->next == NULL && ptr->pid != pid) {  //not found
							puts("pid not exist.");
						} else {
							printf("remove %d %s %d\n", ptr->pid, ptr->name, ptr->timequ);
							prev->next = ptr->next;
							free(ptr->context.uc_stack.ss_sp);
							free(ptr);
						}
					}
				}

				else if(catch[0] == 's')    //start
						break;

					else if(catch[0] == 'p') {  //ps
							tasklist_t *ptr = &head;
							if(ptr->next == NULL) {
								puts("queue is empty.");
							} else {
								while(ptr->next != NULL) {
									ptr = ptr->next;
									printf("%d %s ", ptr->pid, ptr->name);
									if(ptr->state == TASK_RUNNING) {
										printf("TASK_RUNNING");
									} else if(ptr->state == TASK_READY) {
										printf("TASK_READY");
									} else if(ptr->state == TASK_WAITING) {
										printf("TASK_WAITING");
									} else if(ptr->state == TASK_TERMINATED) {
										printf("TASK_TERMINATED");
									}
									printf(" %d\n", ptr->quingtime);
								}
							}
						}
	}
	puts("simulating...");
	setcontext(&Simulator);
	return;
}

void simulator()
{
	// puts("[Simulation Mode]");
	tasklist_t *ptr = &head;

	if(ptr->next == NULL) {	//empty
		puts("queue is empty.");
		setcontext(&Shell);
		return;
	}
	while(ptr->next != NULL) {
		nowfront = ptr;
		ptr = ptr->next;
		if(ptr->state == TASK_RUNNING) {    //if paused continue run it
			nowtask = ptr;
			// printf("continue %d %s\n", ptr->pid, ptr->name);
			if(ptr->timequ == 20) {
				set_timer(0,20000);
			} else {
				set_timer(0,10000);
			}
			// printf("run %d %s\n", nowtask->pid, nowtask->name);
			setcontext(&(ptr->context));
			return;
		}
	}
	ptr = &head;
	while(ptr->next != NULL) {
		nowfront = ptr;
		ptr = ptr->next;
		if(ptr->state == TASK_READY) {  //find the first ready task and run it
			ptr->state = TASK_RUNNING;
			nowtask = ptr;
			if(ptr->timequ == 20) {
				set_timer(0,20000);
			} else {
				set_timer(0,10000);
			}
			// printf("run %d %s\n", ptr->pid, ptr->name);
			setcontext(&(ptr->context));
			return;
		}
	}
	//all task not ready
	ptr = &head;
	while(ptr->next != NULL) {
		nowfront = ptr;
		ptr = ptr->next;
		if(ptr->state == TASK_WAITING) {    //found one waiting task
			nowtask = ptr;
			// puts("waiting...");
			if(ptr->timequ == 20) {
				set_timer(0,20000);
			} else {
				set_timer(0,10000);
			}
			while(1);	//wait timer to decrease suspend time
		}
	}
	puts("all done.");  //all task terminated
	setcontext(&Shell);
	return;
}

int main()
{
	//signal(SIGALRM, tictok);    //timer
	struct sigaction act;
	act.sa_handler = tictok;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGALRM);
	sigaddset(&act.sa_mask, SIGTSTP);
	act.sa_flags = 0;
	if (sigaction(SIGALRM, &act, NULL) < 0)
		return 0;
	//signal(SIGTSTP, pause); //ctrlz
	struct sigaction act2;
	act2.sa_handler = pause;
	sigemptyset(&act2.sa_mask);
	sigaddset(&act2.sa_mask, SIGTSTP);
	sigaddset(&act2.sa_mask, SIGALRM);
	act2.sa_flags = 0;
	if (sigaction(SIGTSTP, &act2, NULL) < 0)
		return 0;

	getcontext(&Shell);
	Shell.uc_link = 0;
	Shell.uc_stack.ss_sp = malloc(4096);
	Shell.uc_stack.ss_size = 4096;
	Shell.uc_stack.ss_flags = 0;
	makecontext(&Shell, (void *)&shell, 0);

	getcontext(&Simulator);
	Simulator.uc_link = 0;
	Simulator.uc_stack.ss_sp = malloc(4096);
	Simulator.uc_stack.ss_size = 4096;
	Simulator.uc_stack.ss_flags = 0;
	makecontext(&Simulator, (void *)&simulator, 0);

	getcontext(&Finish);
	Finish.uc_link = 0;
	Finish.uc_stack.ss_sp = malloc(4096);
	Finish.uc_stack.ss_size = 4096;
	Finish.uc_stack.ss_flags = 0;
	makecontext(&Finish, (void *)&finish, 0);

	setcontext(&Shell);
	return 0;
}
