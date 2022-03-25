#ifndef __BASIC_TYPES_HLSL__
#define __BASIC_TYPES_HLSL__

#ifdef __cplusplus
#ifndef CHECK_STRUCT_ALIGNMENT
#define CHECK_STRUCT_ALIGNMENT(s) static_assert( sizeof(s) % 16 == 0, "sizeof(" #s ") is not multiple of 16" )
#endif
#else
#ifndef CHECK_STRUCT_ALIGNMENT
#define CHECK_STRUCT_ALIGNMENT(s) 
#endif
#endif

// Input structures shared with C++ code
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

    float3 mPosition;
    float mPadding0;

    float3 mViewDirection;
    float mPadding1;
};

CHECK_STRUCT_ALIGNMENT(CameraAttribs);

struct SceneGlobals {
    CameraAttribs mCamera;
};

CHECK_STRUCT_ALIGNMENT(SceneGlobals);

struct StaticInstanceData {
    float4x4 mWorld;
    int mEntity;
    int mPadding0;
    int mPadding1;
    int mPadding2;
};

CHECK_STRUCT_ALIGNMENT(StaticInstanceData);

// Shader only stuff
#if !defined(__cplusplus)
#if !defined(COLOR_ATTRIBUTE)
#define COLOR_ATTRIBUTE_NONE
#define COLOR_ATTRIBUTE_ENABLED false
#else
#define COLOR_ATTRIBUTE_ENABLED true
#endif

#if !defined(DEPTH_ATTRIBUTE)
#define DEPTH_ATTRIBUTE_NONE
#define DEPTH_ATTRIBUTE_ENABLED false
#else
#define DEPTH_ATTRIBUTE_ENABLED true
#endif

#if !defined(ALBEDO_ATTRIBUTE)
#define ALBEDO_ATTRIBUTE_NONE
#define ALBEDO_ATTRIBUTE_ENABLED false
#else
#define ALBEDO_ATTRIBUTE_ENABLED true
#endif

#if !defined(UV_ATTRIBUTE)
#define UV_ATTRIBUTE_NONE
#define UV_ATTRIBUTE_ENABLED false
#else
#define UV_ATTRIBUTE_ENABLED true
#endif

#if !defined(NORMAL_ATTRIBUTE)
#define NORMAL_ATTRIBUTE_NONE
#define NORMAL_ATTRIBUTE_ENABLED false
#else
#define NORMAL_ATTRIBUTE_ENABLED true
#endif

#if !defined(ROUGHNESS_ATTRIBUTE)
#define ROUGHNESS_ATTRIBUTE_NONE
#define ROUGHNESS_ATTRIBUTE_ENABLED false
#else
#define ROUGHNESS_ATTRIBUTE_ENABLED true
#endif

#if !defined(METALLIC_ATTRIBUTE)
#define METALLIC_ATTRIBUTE_NONE
#define METALLIC_ATTRIBUTE_ENABLED false
#else
#define METALLIC_ATTRIBUTE_ENABLED true
#endif

#if !defined(ENTITY_ID_ATTRIBUTE)
#define ENTITY_ID_ATTRIBUTE_NONE
#define ENTITY_ID_ATTRIBUTE_ENABLED false
#else
#define ENTITY_ID_ATTRIBUTE_ENABLED true
#endif

struct PixelData {
    float4 mColor       : COLOR_ATTRIBUTE;
    float mDepth        : DEPTH_ATTRIBUTE;
    float3 mAlbedo      : ALBEDO_ATTRIBUTE;
    float2 mUV          : UV_ATTRIBUTE;
    float3 mNormal      : NORMAL_ATTRIBUTE;
    float3 mRoughness   : ROUGHNESS_ATTRIBUTE;
    float3 mMetallic    : METALLIC_ATTRIBUTE;
    int mEntity         : ENTITY_ID_ATTRIBUTE;
};

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