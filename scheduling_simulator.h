#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include "task.h"

enum TASK_STATE {
	TASK_RUNNING,
	TASK_READY,
	TASK_WAITING,
	TASK_TERMINATED
};

typedef struct tasklist_t {
	int pid;
	char name[10];
	int timequ;
	void (*func)(void);
	int quingtime;
	int susptime;
	int state;
	ucontext_t context;
	struct tasklist_t *next;
} tasklist_t;

void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);

#endif
