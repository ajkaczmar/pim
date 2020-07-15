#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"

PIM_C_BEGIN

// Rayleigh scattering: blue/orange tinting due to air (smaller particles)
// Mies scattering: hazy gray tinting due to aerosols (bigger particles)

typedef struct atmos_s
{
    float crustRadius;
    float atmosphereRadius;
    float3 Br;      // beta rayleigh
    float Bm;       // beta mie
    float Hr;       // rayleigh scale height
    float Hm;       // mie scale height
    float g;        // mean cosine of mie phase function
} atmos_t;

static const atmos_t kEarthAtmosphere =
{
    6360e3f,
    6420e3f,
    { 33.1e-6f, 13.5e-6f, 5.8e-6f },
    21e-6f,
    7994.0f,
    1200.0f,
    0.758f,
};

pim_inline float2 VEC_CALL RaySphereIntersect(float3 ro, float3 rd, float radius)
{
    float a = f3_dot(rd, rd);
    float b = 2.0f * f3_dot(rd, ro);
    float c = f3_dot(ro, ro) - (radius * radius);
    float d = (b * b) - 4.0f * a * c;
    if (d < 0.0f)
    {
        return f2_v(1e5f, -1e5f);
    }
    float sqrtd = sqrtf(d);
    float rcp2a = 1.0f / (2.0f * a);
    return f2_v((-b - sqrtd) * rcp2a, (-b + sqrtd) * rcp2a);
}

pim_inline float VEC_CALL RayleighPhase(float mu)
{
    float mu2 = mu * mu;
    return (3.0f / (16.0f * kPi)) * (1.0f + mu2);
}

pim_inline float VEC_CALL MiePhase(float mu, float g)
{
    float mu2 = mu * mu;
    float g2 = g * g;
    float nom = (1.0f - g2) * (1.0f + mu2);
    float denomBase = (1.0f + g2) - (2.0f * g * mu);
    float denom = (2.0f + g2) * denomBase * sqrtf(denomBase);
    return (3.0f / (8.0f * kPi)) * (nom / denom);
}

// henyey-greenstein phase function
// g: anisotropy in (-1, 1) => (back-scattering, forward-scattering)
// cosTheta: dot(wo, wi)
pim_inline float VEC_CALL HGPhase(float g, float cosTheta)
{
    float g2 = g * g;
    float nom = 1.0f - g2;
    float denom = 1.0f + g2 + 2.0f * g * cosTheta;
    denom = denom * sqrtf(denom);
    return nom / (4.0f * kPi * denom);
}

pim_inline float3 VEC_CALL Atmosphere(
    atmos_t atmos,
    float3 ro,
    float3 rd,
    float3 L,
    float sunIntensity)
{
    const i32 iSteps = 8;
    const i32 jSteps = 4;

    const float rcpHr = 1.0f / atmos.Hr;
    const float rcpHm = 1.0f / atmos.Hm;

    float2 p = RaySphereIntersect(ro, rd, atmos.atmosphereRadius);
    if (p.x > p.y)
    {
        return f3_0;
    }
    p.y = f1_min(p.y, RaySphereIntersect(ro, rd, atmos.crustRadius).x);
    const float iStepSize = (p.y - p.x) / iSteps;
    const float deltaJ = 1.0f / jSteps;

    float3 totalRlh = f3_0;
    float3 totalMie = f3_0;
    float iOdRlh = 0.0;
    float iOdMie = 0.0;
    for (i32 i = 0; i < iSteps; i++)
    {
        float3 iPos = f3_add(ro, f3_mulvs(rd, (i + 0.5f) * iStepSize));
        float jStepSize = RaySphereIntersect(iPos, L, atmos.atmosphereRadius).y * deltaJ;

        float jOdRlh = 0.0;
        float jOdMie = 0.0;
        for (i32 j = 0; j < jSteps; j++)
        {
            float3 jPos = f3_add(iPos, f3_mulvs(L, (j + 0.5f) * jStepSize));
            float jHeight = f3_length(jPos) - atmos.crustRadius;

            jOdRlh += expf(-jHeight * rcpHr) * jStepSize;
            jOdMie += expf(-jHeight * rcpHm) * jStepSize;
        }

        float mieAttn = atmos.Bm * (iOdMie + jOdMie);
        float3 rlhAttn = f3_mulvs(atmos.Br, iOdRlh + jOdRlh);
        float3 attn = f3_exp(f3_neg(f3_addvs(rlhAttn, mieAttn)));

        float iHeight = f3_length(iPos) - atmos.crustRadius;
        float odStepRlh = expf(-iHeight * rcpHr) * iStepSize;
        float odStepMie = expf(-iHeight * rcpHm) * iStepSize;

        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        totalRlh = f3_add(totalRlh, f3_mulvs(attn, odStepRlh));
        totalMie = f3_add(totalMie, f3_mulvs(attn, odStepMie));
    }

    float mu = f3_dot(rd, L);
    totalRlh = f3_mul(totalRlh, f3_mulvs(atmos.Br, RayleighPhase(mu)));
    totalMie = f3_mulvs(totalMie, atmos.Bm * MiePhase(mu, atmos.g));

    return f3_mulvs(f3_add(totalRlh, totalMie), sunIntensity);
}

pim_inline float3 VEC_CALL EarthAtmosphere(
    float3 ro,
    float3 rd,
    float3 L,
    float sunIntensity)
{
    ro.y += kEarthAtmosphere.crustRadius;
    return Atmosphere(
        kEarthAtmosphere,
        ro,
        rd,
        L,
        sunIntensity);
}

PIM_C_END
