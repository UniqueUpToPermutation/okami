#ifndef BASIC_TYPES_HLSL_
#define BASIC_TYPES_HLSL_

struct CameraAttribs {
    float4x4 mView;
    float4x4 mViewProj;
    float4x4 mProj;
    float4x4 mInvView;
    float4x4 mInvViewProj;
    float4x4 mInvProj;
    float2 mViewport;
    float mNearZ;
    float mFarZ;
};

struct SceneGlobals {
    CameraAttribs mCamera;
};

struct StaticInstanceData {
    float4x4 mWorld;
    int mEntity;
    int mPadding0;
    int mPadding1;
    int mPadding2;
};

#if !defined(__cplusplus)
#if defined(SHADERED)
float4 apply(float4x4 mat, float4 vec) {
    return mul(vec, mat);
}
float4 applyT(float4x4 mat, float4 vec) {
    return mul(mat, vec);
}
#define Z_FLIP -1.0
#else
float4 apply(float4x4 mat, float4 vec) {
    return mul(mat, vec);
}
float4 applyT(float4x4 mat, float4 vec) {
    return mul(vec, mat);
}
#define Z_FLIP 1.0
#endif
#endif
#endif