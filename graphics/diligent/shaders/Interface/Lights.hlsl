#ifndef __LIGHTS_COMMON_HLSL__
#define __LIGHTS_COMMON_HLSL__

#include "Common.hlsl"

// Light types
#define HLSL_LIGHT_TYPE_NONE 0
#define HLSL_LIGHT_TYPE_DIRECTIONAL 1
#define HLSL_LIGHT_TYPE_POINT 2

struct LightAttribs {
    float3 mPosition;
    int mLightType;

    float3 mLightDir;
    float mScale;

    float3 mIrradiance;
    float mRadianceFalloff;
};

CHECK_STRUCT_ALIGNMENT(LightAttribs);

#if !defined(__cplusplus)
struct LightSurfaceInfo {
    float3 mDirIn;
    float mRadianceFalloff;
};

void GetLightInfoPoint(
    in LightAttribs light, 
    in float3 surface,
    out LightSurfaceInfo info) {

    float3 diff = surface - light.mPosition;
    info.mDirIn = normalize(diff);
    info.mRadianceFalloff = 1.0 / dot(diff, diff) * light.mScale;
}

void GetLightInfoDirectional(
    in LightAttribs light,
    in float3 surface,
    out LightSurfaceInfo info) {
    
    info.mDirIn = light.mLightDir;
    info.mRadianceFalloff = 1.0;
}

void GetLightInfo(
    in LightAttribs light, 
    in float3 surface,
    out LightSurfaceInfo info) {
    switch (light.mLightType) {
        case HLSL_LIGHT_TYPE_DIRECTIONAL:
            GetLightInfoDirectional(light, surface, info);
        case HLSL_LIGHT_TYPE_POINT:
            GetLightInfoPoint(light, surface, info);
    }
}

#endif
#endif