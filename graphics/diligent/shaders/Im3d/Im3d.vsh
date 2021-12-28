#if !defined(POINTS) && !defined(LINES) && !defined(TRIANGLES)
	#error No primitive type defined
#endif

#include "Types.hlsl"
#include "../BasicTypes.hlsl"

cbuffer cbContextData : register(b0)
{
    SceneGlobals gGlobals;
};

struct VS_INPUT
{
    float4 m_positionSize : ATTRIB0;
    float4 m_color        : ATTRIB1;
};

void main(in VS_INPUT vs_input, out VS_OUTPUT result) 
{
    result.m_color = vs_input.m_color.abgr; // swizzle to correct endianness
    #if !defined(TRIANGLES)
        result.m_color.a *= smoothstep(0.0, 1.0, vs_input.m_positionSize.w / kAntialiasing);
    #endif
    result.m_size = max(vs_input.m_positionSize.w, kAntialiasing);
    result.m_position = mul(gGlobals.mCamera.mViewProj, float4(vs_input.m_positionSize.xyz, 1.0));
}