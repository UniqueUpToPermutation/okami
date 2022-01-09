#include "Common.hlsl"
#include "../../Common.hlsl"

#if defined(SHADERED)
#line __LINE__ + 2
#endif

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
	#if !defined(SHADERED)
    	result.m_color = vs_input.m_color.abgr; // swizzle to correct endianness
    #else 
    	result.m_color = vs_input.m_color;
    #endif
    
   	#if !defined(TRIANGLES)
        result.m_color.a *= smoothstep(0.0, 1.0, vs_input.m_positionSize.w / kAntialiasing);
    #endif
    
    #if !defined(LINES)
    	result.m_position = apply(
	        	gGlobals.mCamera.mViewProj,
	        	float4(vs_input.m_positionSize.xyz, 1.0)
	    );
    #else 
	    result.m_position = apply(
	        	gGlobals.mCamera.mView,
	        	float4(vs_input.m_positionSize.xyz, 1.0)
	    );
    #endif
    
    result.m_size = max(vs_input.m_positionSize.w, kAntialiasing);
}