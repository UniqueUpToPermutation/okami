#ifndef __MATERIALS_COMMON_HLSL__
#define __MATERIALS_COMMON_HLSL__

#include "Common.hlsl"

#define MATERIAL_TYPE_FLAT 0
#define MATERIAL_TYPE_LAMBERT 1
#define MATERIAL_TYPE_PHONG 2
#define MATERIAL_TYPE_COOK_TORRENCE 3

#if defined(__cplusplus)
enum class HLSLMaterialType {
    Flat = MATERIAL_TYPE_FLAT,
    Lambert = MATERIAL_TYPE_LAMBERT,
    Phong = MATERIAL_TYPE_PHONG,
    CookTorrence = MATERIAL_TYPE_COOK_TORRENCE
};
#endif

struct MaterialDesc {
    float3 mAlbedoFactor;
    float mSpecularPower;

    float mRoughnessFactor;
    float mMetallicFactor;
    int mMaterialType;
    int mPadding0;
};

CHECK_STRUCT_ALIGNMENT(MaterialDesc);

#endif