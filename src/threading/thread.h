#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct thread_s { void* handle; } thread_t;
typedef i32 (PIM_CDECL *thread_fn)(void* data);

void thread_create(thread_t* tr, thread_fn entrypoint, void* data);
void thread_join(thread_t* tr);
void thread_set_aff(thread_t* tr, u64 mask);
void thread_set_priority(thread_t* tr, i32 priority);
i32 thread_hardware_count(void);

PIM_C_END
