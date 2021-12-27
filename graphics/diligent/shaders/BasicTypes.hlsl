#ifndef BASIC_TYPES_HLSL_
#define BASIC_TYPES_HLSL_

struct CameraAttribs {
    float4x4 mView;
    float4x4 mViewProj;
    float4x4 mInvView;
    float4x4 mInvViewProj;
    float2 mViewport;
    float2 mPadding0;
};

struct SceneGlobals {
    CameraAttribs mCamera;
};

struct StaticInstanceData {
    float4x4 mWorld;
};

#endif