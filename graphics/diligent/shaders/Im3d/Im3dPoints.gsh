#if !defined(POINTS) && !defined(LINES) && !defined(TRIANGLES)
	#error No primitive type defined
#endif

#include "Types.hlsl"
#include "../BasicTypes.hlsl"

cbuffer cbContextData : register(b0)
{
    SceneGlobals gGlobals;
};

[maxvertexcount(4)]
void main(point VS_OUTPUT gs_input[1], inout TriangleStream<VS_OUTPUT> result)
{
    VS_OUTPUT ret;
    
    float2 scale = 1.0 / gGlobals.mCamera.mViewport * gs_input[0].m_size;
    ret.m_size  = gs_input[0].m_size;
    ret.m_color = gs_input[0].m_color;
    ret.m_edgeDistance = gs_input[0].m_edgeDistance;
    
    ret.m_position = float4(gs_input[0].m_position.xy + float2(-1.0, -1.0) * scale * gs_input[0].m_position.w, gs_input[0].m_position.zw);
    ret.m_uv = float2(0.0, 0.0);
    result.Append(ret);
    
    ret.m_position = float4(gs_input[0].m_position.xy + float2(-1.0,  1.0) * scale * gs_input[0].m_position.w, gs_input[0].m_position.zw);
    ret.m_uv = float2(0.0, 1.0);
    result.Append(ret);

    ret.m_position = float4(gs_input[0].m_position.xy + float2( 1.0, -1.0) * scale * gs_input[0].m_position.w, gs_input[0].m_position.zw);
    ret.m_uv = float2(1.0, 0.0);
    result.Append(ret);
    
    ret.m_position = float4(gs_input[0].m_position.xy + float2( 1.0,  1.0) * scale * gs_input[0].m_position.w, gs_input[0].m_position.zw);
    ret.m_uv = float2(1.0, 1.0);
    result.Append(ret);
}
