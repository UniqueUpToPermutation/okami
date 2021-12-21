#pragma once

struct SceneGlobals {
#ifdef __cplusplus
    Diligent::float4x4 mView;
    Diligent::float4x4 mViewProj;
    Diligent::float4x4 mInvView;
    Diligent::float4x4 mInvViewProj;
#else 
    matrix mView;
    matrix mViewProj;
    matrix mInvView;
    matrix mInvViewProj;
#endif
};

struct StaticInstanceData {
#ifdef __cplusplus
    Diligent::float4x4 mWorld;
#else
    matrix mWorld;
#endif
};