#include "mtask.h"
#include "asmfunc.h"
#include "desctable.h"
#include "timer.h"

struct TASKCTL* taskctl;
struct TIMER* task_timer;

struct TASK* task_init(struct MEMMAN* memman) {
    struct TASK *task, *idle;
    struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)ADDR_GDT;

    taskctl = (struct TASKCTL*)memman_alloc_4k(memman, sizeof(struct TASKCTL));

    for (int i = 0; i < MAX_TASKS; ++i) {
        taskctl->tasks0[i].flags = TASK_STATE_STOPPED;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) << 3;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
    }

    for (int i = 0; i < MAX_TASK_LEVELS; ++i) {
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }

    task = task_alloc();
    task->flags = TASK_STATE_RUNNING;
    task->priority = 2;
    task->level = 0;
    task_add(task);
    task_switchsub(); // setting level
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);

    idle = task_alloc();
    idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
    idle->tss.eip = (int)&task_idle;
    idle->tss.es = 1 * 8;
    idle->tss.cs = 2 * 8;
    idle->tss.ss = idle->tss.ds = idle->tss.fs = idle->tss.gs = 1 * 8;
    task_run(idle, MAX_TASK_LEVELS - 1, 1);

    return task;
}

struct TASK* task_alloc(void) {
    struct TASK* task;
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (taskctl->tasks0[i].flags != TASK_STATE_STOPPED) continue;
        task = &taskctl->tasks0[i];
        task->flags = TASK_STATE_WAITING;
        task->tss.eflags = 0x00000202; // IF = 1
        task->tss.eax = task->tss.ecx = task->tss.edx = task->tss.ebx = 0;
        task->tss.ebp = task->tss.esi = task->tss.edi = 0;
        task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = 0;
        task->tss.ldtr = 0;
        task->tss.iomap = 0x40000000;
        task->tss.ss0 = 0;
        return task;
    }
    return NULL;
}

// priority == 0 : don't change priority
// level < 0 : don't change level
void task_run(struct TASK* task, int level, int priority) {
    if (level < 0) level = task->level;
    if (priority > 0) task->priority = priority;
    if (task->flags == TASK_STATE_RUNNING && task->level != level) {
        task_remove(task); // if removed flags will be WAITING
    }
    if (task->flags != TASK_STATE_RUNNING) {
        // returning from sleep
        task->level = level;
        task_add(task);
    }
    taskctl->lv_change = TRUE; // check level in the next switch
    return;
}

void task_switch(void) {
    struct TASKLEVEL* tl = &taskctl->level[taskctl->now_lv];
    struct TASK *new_task, *now_task = tl->tasks[tl->now++];
    tl->now %= tl->running;
    if (taskctl->lv_change) {
        task_switchsub();
        tl = &taskctl->level[taskctl->now_lv];
    }
    new_task = tl->tasks[tl->now];
    timer_settime(task_timer, new_task->priority);
    if (new_task != now_task) farjmp(0, new_task->sel);
    return;
}

void task_sleep(struct TASK* task) {
    struct TASK* now_task;
    if (task->flags == TASK_STATE_RUNNING) {
        now_task = task_now();
        task_remove(task); // flags of task will be WAITING
        if (task == now_task) {
            task_switchsub();
            now_task = task_now();
            farjmp(0, now_task->sel);
        }
    }
    return;
}

struct TASK* task_now(void) {
    struct TASKLEVEL* tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now];
}

void task_add(struct TASK* task) {
    struct TASKLEVEL* tl = &taskctl->level[task->level];
    tl->tasks[tl->running++] = task;
    task->flags = TASK_STATE_RUNNING;
    return;
}

void task_remove(struct TASK* task) {
    struct TASKLEVEL* tl = &taskctl->level[task->level];
    int i = 0;
    while (i < tl->running && tl->tasks[i] != task) ++i;
    tl->running--;
    if (i < tl->now) tl->now--;
    if (tl->now >= tl->running) tl->now = 0;
    task->flags = TASK_STATE_WAITING;
    while (i < tl->running) tl->tasks[i] = tl->tasks[i + 1], ++i;
    return;
}

void task_switchsub(void) {
    int i = 0;
    while (i < MAX_TASK_LEVELS && taskctl->level[i].running <= 0) ++i;
    taskctl->now_lv = i;
    taskctl->lv_change = FALSE;
    return;
}

void task_idle(void) {
    while (TRUE) io_hlt();
}