#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/sampler.h"
#include "math/frustum.h"

PIM_C_BEGIN

#define CUBEMAP_DEFAULT_SIZE    64      // pow2(CUBEMAP_MAX_MIP)
#define CUBEMAP_MAX_MIP         6.0f    // log2(CUBEMAP_DEFAULT_SIZE)

typedef struct pt_scene_s pt_scene_t;

typedef enum
{
    Cubeface_XP,
    Cubeface_XM,
    Cubeface_YP,
    Cubeface_YM,
    Cubeface_ZP,
    Cubeface_ZM,

    Cubeface_COUNT
} Cubeface;

typedef struct Cubemap_s
{
    i32 size;
    i32 mipCount;
    float3* color[Cubeface_COUNT];
    float4* convolved[Cubeface_COUNT];
} Cubemap;

typedef struct Cubemaps_s
{
    i32 count;
    u32* names;
    Cubemap* cubemaps;
    sphere_t* bounds;
} Cubemaps_t;

Cubemaps_t* Cubemaps_Get(void);

i32 Cubemaps_Add(u32 name, i32 size, sphere_t bounds);
bool Cubemaps_Rm(u32 name);
i32 Cubemaps_Find(u32 name);

void Cubemap_New(Cubemap* cm, i32 size);
void Cubemap_Del(Cubemap* cm);

Cubeface VEC_CALL Cubemap_CalcUv(float4 dir, float2* uvOut);

// note: input is roughness, not alpha (aka roughness^2)
pim_inline float VEC_CALL RoughnessToMip(float roughness)
{
    return roughness * CUBEMAP_MAX_MIP;
}

// note: output is roughness, not alpha (aka roughness^2)
pim_inline float VEC_CALL MipToRoughness(float mip)
{
    return mip / CUBEMAP_MAX_MIP;
}

static const float4 Cubemap_kForwards[] =
{
    {1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, -1.0f},
};

static const float4 Cubemap_kUps[] =
{
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
};

static const float4 Cubemap_kRights[] =
{
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
};

pim_inline Cubeface VEC_CALL Cubemap_CalcUv(float4 dir, float2* uvOut)
{
    ASSERT(uvOut);

    float4 absdir = f4_abs(dir);
    float v = f4_hmax3(absdir);
    float ma = 0.5f / v;

    Cubeface face;
    if (v == absdir.x)
    {
        face = dir.x < 0.0f ? Cubeface_XM : Cubeface_XP;
    }
    else if (v == absdir.y)
    {
        face = dir.y < 0.0f ? Cubeface_YM : Cubeface_YP;
    }
    else
    {
        face = dir.z < 0.0f ? Cubeface_ZM : Cubeface_ZP;
    }

    float2 uv;
    uv.x = f4_dot3(Cubemap_kRights[face], dir);
    uv.y = f4_dot3(Cubemap_kUps[face], dir);
    uv = f2_addvs(f2_mulvs(uv, ma), 0.5f);

    *uvOut = uv;
    return face;
}

pim_inline float4 VEC_CALL Cubemap_ReadConvolved(const Cubemap* cm, float4 dir, float mip)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    mip = f1_clamp(mip, 0.0f, (float)(cm->mipCount - 1));
    int2 size = { cm->size, cm->size };
    const float4* pim_noalias buffer = cm->convolved[face];
    ASSERT(buffer);

    return TrilinearClamp_f4(buffer, size, uv, mip);
}

pim_inline float3 VEC_CALL Cubemap_ReadColor(const Cubemap* cm, float4 dir)
{
    ASSERT(cm);

    float2 uv;
    Cubeface face = Cubemap_CalcUv(dir, &uv);

    int2 size = { cm->size, cm->size };
    const float3* pim_noalias buffer = cm->color[face];
    ASSERT(buffer);

    return UvBilinearClamp_f3(buffer, size, uv);
}

pim_inline void VEC_CALL Cubemap_WriteColor(Cubemap* cm, Cubeface face, int2 coord, float3 value)
{
    ASSERT(cm);
    float3* pim_noalias buffer = cm->color[face];
    ASSERT(buffer);
    i32 i = Clamp(i2_s(cm->size), coord);
    buffer[i] = value;
}

pim_inline void VEC_CALL Cubemap_WriteConvolved(Cubemap* cm, Cubeface face, int2 coord, float4 value)
{
    ASSERT(cm);
    float4* pim_noalias buffer = cm->convolved[face];
    ASSERT(buffer);
    i32 i = Clamp(i2_s(cm->size), coord);
    buffer[i] = value;
}

pim_inline void VEC_CALL Cubemap_WriteMip(
    Cubemap* cm,
    Cubeface face,
    int2 coord,
    i32 mip,
    float4 value)
{
    ASSERT(cm);
    float4* pim_noalias buffer = cm->convolved[face];
    ASSERT(buffer);

    int2 size = i2_s(cm->size);
    i32 offset = CalcMipOffset(size, mip);
    buffer += offset;

    int2 mipSize = CalcMipSize(size, mip);
    i32 i = Clamp(mipSize, coord);
    buffer[i] = value;
}

pim_inline void VEC_CALL Cubemap_BlendMip(
    Cubemap* cm,
    Cubeface face,
    int2 coord,
    i32 mip,
    float4 value,
    float weight)
{
    ASSERT(cm);
    float4* pim_noalias buffer = cm->convolved[face];
    ASSERT(buffer);

    int2 size = i2_s(cm->size);
    i32 offset = CalcMipOffset(size, mip);
    buffer += offset;

    int2 mipSize = CalcMipSize(size, mip);
    i32 i = Clamp(mipSize, coord);
    buffer[i] = f4_lerpvs(buffer[i], value, weight);
}

pim_inline float4 VEC_CALL Cubemap_CalcDir(
    i32 size,
    Cubeface face,
    int2 coord,
    float2 Xi)
{
    float2 uv = CoordToUv(i2_s(size), coord);
    Xi = f2_mulvs(Xi, 1.0f / size);
    uv = f2_add(uv, Xi);
    uv = f2_snorm(uv);

    float4 fwd = Cubemap_kForwards[face];
    float4 right = Cubemap_kRights[face];
    float4 up = Cubemap_kUps[face];
    float4 dir = proj_dir(right, up, fwd, f2_1, uv);

    return dir;
}

void Cubemap_Bake(
    Cubemap* cm,
    pt_scene_t* scene,
    float4 origin,
    float weight);

void Cubemap_Convolve(
    Cubemap* cm,
    u32 sampleCount,
    float weight);

PIM_C_END
