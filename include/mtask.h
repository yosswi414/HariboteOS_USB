#pragma once

#include "fifo.h"
#include "memory.h"

#define MAX_TASKS 1000
#define TASK_GDT0 3  // beginning point of TSS in GDT

#define TASK_STATE_STOPPED 0
#define TASK_STATE_WAITING 1
#define TASK_STATE_RUNNING 2

#define MAX_TASK_INLVTASKS 100
#define MAX_TASK_LEVELS 10

// Task Status Segment
struct TSS32 {
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
};

struct TASK {
    int sel, flags;
    int level, priority;
    struct FIFO32 fifo;
    struct TSS32 tss;
    struct CONSOLE* cons;
    int ds_base, cons_stack;
};

struct TASKLEVEL {
    int running;  // number of tasks running
    int now;      // indicates a task currently being processed
    struct TASK* tasks[MAX_TASK_INLVTASKS];
};

struct TASKCTL {
    int now_lv;      // level running
    char lv_change;  // if true, level will be changed in the next switch
    struct TASKLEVEL level[MAX_TASK_LEVELS];
    struct TASK tasks0[MAX_TASKS];
};

struct TASK* task_init(struct MEMMAN* memman);
struct TASK* task_alloc(void);
void task_run(struct TASK* task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK* task);
struct TASK* task_now(void);
void task_add(struct TASK* task);
void task_remove(struct TASK* task);
void task_switchsub(void);
void task_idle(void);
