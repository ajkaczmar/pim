#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "math/types.h"
#include "math/float4_funcs.h"

pim_inline u16 VEC_CALL f4_rgb5a1(float4 v)
{
    v = f4_saturate(v);
    v = f4_mulvs(v, 31.0f);
    u16 r = (u16)v.x;
    u16 g = (u16)v.y;
    u16 b = (u16)v.z;
    u16 c = (r << 11) | (g << 6) | (b << 1) | 1;
    return c;
}

pim_inline u32 VEC_CALL f4_rgba8(float4 v)
{
    v = f4_saturate(v);
    v = f4_mulvs(v, 255.0f);
    u32 r = (u32)v.x;
    u32 g = (u32)v.y;
    u32 b = (u32)v.z;
    u32 a = (u32)v.w;
    u32 c = (a << 24) | (b << 16) | (g << 8) | r;
    return c;
}

pim_inline float4 VEC_CALL rgba8_f4(u32 c)
{
    const float s = 1.0f / 255.0f;
    u32 r = c & 0xff;
    c >>= 8;
    u32 g = c & 0xff;
    c >>= 8;
    u32 b = c & 0xff;
    c >>= 8;
    u32 a = c & 0xff;
    return f4_v(r * s, g * s, b * s, a * s);
}

// reference sRGB -> Linear conversion (no approximation)
pim_inline float VEC_CALL sRGBToLinear(float c)
{
    if (c <= 0.04045f)
    {
        return c / 12.92f;
    }
    return powf((c + 0.055f) / 1.055f, 2.4f);
}

// reference Linear -> sRGB conversion (no approximation)
pim_inline float VEC_CALL LinearTosRGB(float c)
{
    if (c <= 0.0031308f)
    {
        return c * 12.92f;
    }
    return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

// cubic fit sRGB -> Linear conversion
pim_inline float VEC_CALL f1_tolinear(float c)
{
    float c2 = c * c;
    float c3 = c2 * c;
    return 0.016124f * c + 0.667737f * c2 + 0.317955f * c3;
}

// cubic fit sRGB -> Linear conversion
pim_inline float4 VEC_CALL f4_tolinear(float4 srgb)
{
    float4 lin;
    lin.x = f1_tolinear(srgb.x);
    lin.y = f1_tolinear(srgb.y);
    lin.z = f1_tolinear(srgb.z);
    lin.w = f1_tolinear(srgb.w);
    return lin;
}

// cubic root fit Linear -> sRGB conversion
pim_inline float VEC_CALL f1_tosrgb(float c)
{
    float s1 = sqrtf(c);
    float s2 = sqrtf(s1);
    float s3 = sqrtf(s2);
    return 0.582176f * s1 + 0.789747f * s2 - 0.371449f * s3;
}

// cubic root fit Linear -> sRGB conversion
pim_inline float4 VEC_CALL f4_tosrgb(float4 lin)
{
    float4 srgb;
    srgb.x = f1_tosrgb(lin.x);
    srgb.y = f1_tosrgb(lin.y);
    srgb.z = f1_tosrgb(lin.z);
    srgb.w = f1_tosrgb(lin.w);
    return srgb;
}

pim_inline float VEC_CALL tmap1_reinhard(float x)
{
    float y = x / (1.0f + x);
    return y; // take to srgb in tmap4
}

pim_inline float4 VEC_CALL tmap4_reinhard(float4 x)
{
    float4 y = f4_div(x, f4_addvs(x, 1.0f));
    return f4_tosrgb(y);
}

pim_inline float VEC_CALL tmap1_aces(float x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    float y = (x * (a * x + b)) / (x * (c * x + d) + e);
    return y; // take to srgb in tmap4
}

pim_inline float4 VEC_CALL tmap4_aces(float4 x)
{
    float4 y;
    y.x = tmap1_aces(x.x);
    y.y = tmap1_aces(x.y);
    y.z = tmap1_aces(x.z);
    y.w = tmap1_aces(x.w);
    return f4_tosrgb(y);
}

pim_inline float VEC_CALL tmap1_filmic(float x)
{
    x = f1_max(0.0f, x - 0.004f);
    float y = (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
    return y; // originally fit to gamma2.2
}

pim_inline float4 VEC_CALL tmap4_filmic(float4 x)
{
    float4 y;
    y.x = tmap1_filmic(x.x);
    y.y = tmap1_filmic(x.y);
    y.z = tmap1_filmic(x.z);
    y.w = tmap1_filmic(x.w);
    // filmic is fit to gamma2.2
    return y;
}

pim_inline float VEC_CALL tmap1_uchart2(float x)
{
    const float a = 0.15f;
    const float b = 0.50f;
    const float c = 0.10f;
    const float d = 0.20f;
    const float e = 0.02f;
    const float f = 0.30f;
    float y = ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
    return y; // take to srgb in tmap4
}

pim_inline float4 VEC_CALL tmap4_uchart2(float4 x)
{
    const float w = 11.2f;
    x = f4_mulvs(x, 2.0f);
    float4 y;
    y.x = tmap1_uchart2(x.x);
    y.y = tmap1_uchart2(x.y);
    y.z = tmap1_uchart2(x.z);
    y.w = tmap1_uchart2(w);
    y = f4_divvs(y, y.w);
    return f4_tosrgb(y);
}

// params: Shoulder Strength, Linear Strength, Linear Angle, Toe Strength
pim_inline float VEC_CALL tmap1_hable(float x, float4 params)
{
    const float A = params.x;
    const float B = params.y;
    const float C = params.z;
    const float D = params.w;
    const float E = 0.02f;
    const float F = 0.3f;
    float y = ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    return y; // take to srgb in tmap4
}

pim_inline float4 VEC_CALL tmap4_hable(float4 x, float4 params)
{
    const float w = 11.2f;
    x = f4_mulvs(x, 2.0f);
    float4 y;
    y.x = tmap1_hable(x.x, params);
    y.y = tmap1_hable(x.y, params);
    y.z = tmap1_hable(x.z, params);
    y.w = tmap1_hable(w, params);
    y = f4_divvs(y, y.w);
    return f4_tosrgb(y);
}

PIM_C_END