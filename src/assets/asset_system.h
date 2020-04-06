#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct asset_s
{
    const char* name;
    const char* pack;
    i32 offset;
    i32 size;
    const void* pData;
} asset_t;

void asset_sys_init(void);
void asset_sys_update(void);
void asset_sys_shutdown(void);

i32 asset_sys_get(const char* name, asset_t* assetOut);

PIM_C_END
