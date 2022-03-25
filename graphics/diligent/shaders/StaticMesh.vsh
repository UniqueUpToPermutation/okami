#include "Interface/Common.hlsl"

#include "Utils/VertexProcessing.hlsl"

cbuffer cbuf_SceneGlobals {
    SceneGlobals gGlobals;
};

cbuffer cbuf_InstanceData {
    StaticInstanceData gInstanceData;
};

struct VSInput {
    float3 Pos      : ATTRIB0;
    float2 UV       : ATTRIB1;
    float3 Normal   : ATTRIB2;
};

struct PSInput { 
    float4 Pos          : SV_POSITION;
    float3 WorldPos     : POSITION;
    float3 Normal       : NORMAL;
    float2 UV           : TEXCOORD0;
};

void main(in VSInput vs_input,
          out PSInput vs_output)
{
    float4x4 world = gInstanceData.mWorld;
    float4x4 viewProj = gGlobals.mCamera.mViewProj;

    TransformedVertex vert = TransformVertex(
        vs_input.Pos, 
        vs_input.Normal, 
        world);

    vs_output.WorldPos  = vert.WorldPos;
    vs_output.Pos       = mul(viewProj, float4(vert.WorldPos, 1.0));
    vs_output.UV        = vs_input.UV;
    vs_output.Normal    = vert.Normal;
}