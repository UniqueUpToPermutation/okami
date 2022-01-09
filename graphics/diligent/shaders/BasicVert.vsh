#include "Common.hlsl"

cbuffer cbuf_SceneGlobals {
    SceneGlobals gGlobals;
};

cbuffer cbuf_InstanceData {
    StaticInstanceData gInstanceData;
};

struct VSInput {
    float3 Pos  : ATTRIB0;
    float2 UV   : ATTRIB1;
};

struct PSInput { 
    float4 Pos   : SV_POSITION;
    float2 UV    : TEXCOORD0;
};

void main(in VSInput vs_input,
          out PSInput vs_output) 
{
    vs_output.Pos    = mul(gGlobals.mCamera.mViewProj, 
        mul(gInstanceData.mWorld, float4(vs_input.Pos, 1.0)));
    vs_output.UV     = vs_input.UV;
}