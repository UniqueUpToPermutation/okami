#ifndef __BRDFS_HLSL__
#define __BRDFS_HLSL__

#include "../Interface/Common.hlsl"
#include "../Interface/Material.hlsl"

struct BRDFInput {
    float3 mAlbedo;
    float mMetalness;
    float3 mNormal;
    float mRoughness;

    float3 mEyeDirOut;
    float3 mLightDirIn;
    float3 mRadiance;

    float mSpecularPower;
    float3 mSpecularColor;
    
    int mMaterialType;
};

float3 LambertDirect(BRDFInput input) {
    float NdotL = dot(input.mNormal, -input.mLightDirIn);

    float3 color = float3(0.0, 0.0, 0.0);

    color += max(NdotL, 0.0) * input.mAlbedo * input.mRadiance;

    return color;
}

float3 PhongDirect(BRDFInput input) {
    float NdotL = dot(input.mNormal, -input.mLightDirIn);

    float3 R = reflect(-input.mLightDirIn, input.mNormal);

    float3 color = float3(0.0, 0.0, 0.0);

    color += max(NdotL, 0.0) * input.mAlbedo * input.mRadiance;
    float RdotV = -dot(R, input.mEyeDirOut);
    color += input.mRadiance * input.mSpecularColor * 
        pow(max(0.0, RdotV), input.mSpecularPower);

    return color;
}

float3 DirectContribution(BRDFInput input) {
    switch (input.mMaterialType) {
        case MATERIAL_TYPE_FLAT:
            return float3(0.0, 0.0, 0.0);
        case MATERIAL_TYPE_LAMBERT:
            return LambertDirect(input);
        case MATERIAL_TYPE_PHONG:
            return PhongDirect(input);
        case MATERIAL_TYPE_COOK_TORRENCE:
            return float3(0.0, 0.0, 0.0);
    }
}

#endif