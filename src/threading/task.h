#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    TaskStatus_Init = 0,
    TaskStatus_Exec,
    TaskStatus_Complete,
} TaskStatus;

typedef void(PIM_CDECL *task_execute_fn)(struct task_s* task, i32 begin, i32 end);

typedef struct task_s
{
    task_execute_fn execute;
    i32 status;
    i32 worksize;
    i32 head;
    i32 tail;
} task_t;

i32 task_thread_id(void);
i32 task_thread_ct(void);
i32 task_num_active(void);

void task_submit(task_t* task, task_execute_fn execute, i32 worksize);
TaskStatus task_stat(const task_t* task);
void task_await(const task_t* task);
i32 task_poll(const task_t* task);

void task_run(task_t* task, task_execute_fn fn, i32 worksize);

void task_sys_schedule(void);

void task_sys_init(void);
void task_sys_update(void);
void task_sys_shutdown(void);

PIM_C_END
