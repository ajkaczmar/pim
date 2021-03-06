#include "rendering/cubemap.h"
#include "allocator/allocator.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/quat_funcs.h"
#include "common/random.h"
#include "threading/task.h"
#include "rendering/path_tracer.h"
#include "rendering/denoise.h"
#include "math/sampling.h"
#include "common/profiler.h"
#include "common/find.h"
#include <string.h>

static Cubemaps_t ms_cubemaps;

Cubemaps_t* Cubemaps_Get(void) { return &ms_cubemaps; }

i32 Cubemaps_Add(u32 name, i32 size, sphere_t bounds)
{
    const i32 back = ms_cubemaps.count;
    const i32 len = back + 1;
    ms_cubemaps.count = len;

    PermGrow(ms_cubemaps.names, len);
    PermGrow(ms_cubemaps.bounds, len);
    PermGrow(ms_cubemaps.cubemaps, len);

    ms_cubemaps.names[back] = name;
    ms_cubemaps.bounds[back] = bounds;
    Cubemap_New(ms_cubemaps.cubemaps + back, size);

    return back;
}

bool Cubemaps_Rm(u32 name)
{
    const i32 i = Cubemaps_Find(name);
    if (i == -1)
    {
        return false;
    }

    Cubemap_Del(ms_cubemaps.cubemaps + i);
    const i32 len = ms_cubemaps.count;
    ms_cubemaps.count = len - 1;

    PopSwap(ms_cubemaps.names, i, len);
    PopSwap(ms_cubemaps.bounds, i, len);
    PopSwap(ms_cubemaps.cubemaps, i, len);

    return true;
}

i32 Cubemaps_Find(u32 name)
{
    return Find_u32(ms_cubemaps.names, ms_cubemaps.count, name);
}

void Cubemap_New(Cubemap* cm, i32 size)
{
    ASSERT(cm);
    ASSERT(size >= 0);

    const i32 len = size * size;
    const int2 s = { size, size };
    const i32 mipCount = CalcMipCount(s);
    const i32 elemCount = CalcMipOffset(s, mipCount) + 1;

    cm->size = size;
    cm->mipCount = mipCount;
    for (i32 i = 0; i < Cubeface_COUNT; ++i)
    {
        cm->color[i] = perm_calloc(sizeof(cm->color[0][0]) * len);
        cm->convolved[i] = perm_calloc(sizeof(cm->convolved[0][0]) * elemCount);
    }
}

void Cubemap_Del(Cubemap* cm)
{
    if (cm)
    {
        for (i32 i = 0; i < Cubeface_COUNT; ++i)
        {
            pim_free(cm->color[i]);
            pim_free(cm->convolved[i]);
        }
        memset(cm, 0, sizeof(*cm));
    }
}

typedef struct cmbake_s
{
    task_t;
    Cubemap* cm;
    pt_scene_t* scene;
    float4 origin;
    float weight;
} cmbake_t;

pim_optimize
static void BakeFn(task_t* pBase, i32 begin, i32 end)
{
    const i32 tid = task_thread_id();

    cmbake_t* task = (cmbake_t*)pBase;

    Cubemap* cm = task->cm;
    pt_scene_t* scene = task->scene;
    const float4 origin = task->origin;
    const float weight = task->weight;

    const i32 size = cm->size;
    const i32 flen = size * size;

    pt_sampler_t sampler = pt_sampler_get();
    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / flen;
        i32 fi = i % flen;
        int2 coord = { fi % size, fi / size };
        float2 Xi = f2_tent(pt_sample_2d(&sampler));
        float4 dir = Cubemap_CalcDir(size, face, coord, Xi);
        ray_t ray = { origin, dir };
        pt_result_t result = pt_trace_ray(&sampler, scene, ray);
        cm->color[face][fi] = f3_lerp(cm->color[face][fi], result.color, weight);
    }
    pt_sampler_set(sampler);
}

ProfileMark(pm_Bake, Cubemap_Bake)
void Cubemap_Bake(
    Cubemap* cm,
    pt_scene_t* scene,
    float4 origin,
    float weight)
{
    ASSERT(cm);
    ASSERT(scene);
    ASSERT(weight > 0.0f);

    i32 size = cm->size;
    if (size > 0)
    {
        ProfileBegin(pm_Bake);

        cmbake_t* task = tmp_calloc(sizeof(*task));
        task->cm = cm;
        task->scene = scene;
        task->origin = origin;
        task->weight = weight;

        task_run((task_t*)task, BakeFn, size * size * Cubeface_COUNT);

        ProfileEnd(pm_Bake);
    }
}

pim_optimize
static float4 VEC_CALL PrefilterEnvMap(
    const Cubemap* cm,
    float3x3 TBN,
    float4 N,
    float2 offset,
    u32 sampleCount,
    float roughness)
{
    // N=V approximation
    float4 I = f4_neg(N);

    float weight = 0.0f;
    float4 light = f4_0;
    prng_t rng = prng_get();
    for (u32 i = 0; i < sampleCount; ++i)
    {
        float2 Xi = Hammersley2D(i, sampleCount);
        Xi.x = fmodf(Xi.x + offset.x, 1.0f);
        Xi.y = fmodf(Xi.y + offset.y, 1.0f);

        float4 H = TbnToWorld(TBN, SampleGGXMicrofacet(Xi, roughness));
        float4 L = f4_normalize3(f4_reflect3(I, H));
        float NoL = f4_dot3(L, N);
        if (NoL > 0.0f)
        {
            float4 sample = f3_f4(Cubemap_ReadColor(cm, L), 1.0f);
            sample = f4_mulvs(sample, NoL);
            light = f4_add(light, sample);
            weight += NoL;
        }
    }
    prng_set(rng);
    if (weight > 0.0f)
    {
        light = f4_divvs(light, weight);
    }
    return light;
}

typedef struct prefilter_s
{
    task_t task;
    Cubemap* cm;
    i32 mip;
    i32 size;
    u32 sampleCount;
    float weight;
    float2 offset;
} prefilter_t;

pim_optimize
static void PrefilterFn(task_t* pBase, i32 begin, i32 end)
{
    prefilter_t* task = (prefilter_t*)pBase;
    Cubemap* cm = task->cm;

    const u32 sampleCount = task->sampleCount;
    const float weight = task->weight;
    const float2 offset = task->offset;
    const i32 size = task->size;
    const i32 mip = task->mip;
    const i32 len = size * size;
    const float roughness = MipToRoughness((float)mip);

    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / len;
        i32 fi = i % len;
        ASSERT(face < Cubeface_COUNT);
        int2 coord = { fi % size, fi / size };

        float4 N = Cubemap_CalcDir(size, face, coord, f2_0);
        float3x3 TBN = NormalToTBN(N);

        float4 light = PrefilterEnvMap(cm, TBN, N, offset, sampleCount, roughness);
        Cubemap_BlendMip(cm, face, coord, mip, light, weight);
    }
}

ProfileMark(pm_Convolve, Cubemap_Convolve)
void Cubemap_Convolve(
    Cubemap* cm,
    u32 sampleCount,
    float weight)
{
    ASSERT(cm);

    ProfileBegin(pm_Convolve);

    const i32 mipCount = cm->mipCount;
    const i32 size = cm->size;

    prng_t rng = prng_get();
    float2 offset = f2_rand(&rng);
    prng_set(rng);

    task_t** tasks = tmp_calloc(sizeof(tasks[0]) * mipCount);
    for (i32 m = 0; m < mipCount; ++m)
    {
        i32 mSize = size >> m;
        i32 len = mSize * mSize * Cubeface_COUNT;

        if (len > 0)
        {
            prefilter_t* task = tmp_calloc(sizeof(*task));
            task->cm = cm;
            task->mip = m;
            task->size = mSize;
            task->sampleCount = sampleCount;
            task->weight = weight;
            task->offset = offset;
            tasks[m] = (task_t*)task;
            task_submit((task_t*)task, PrefilterFn, len);
        }
    }

    task_sys_schedule();

    for (i32 m = 0; m < mipCount; ++m)
    {
        if (tasks[m])
        {
            task_await(tasks[m]);
        }
    }

    ProfileEnd(pm_Convolve);
}
