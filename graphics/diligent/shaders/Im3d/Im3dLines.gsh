#if !defined(POINTS) && !defined(LINES) && !defined(TRIANGLES)
	#error No primitive type defined
#endif

#include "Types.hlsl"
#include "../BasicTypes.hlsl"

cbuffer cbContextData : register(b0)
{
    SceneGlobals gGlobals;
};

// expand line -> triangle strip
[maxvertexcount(4)]
void main(line VS_OUTPUT input[2], inout TriangleStream<VS_OUTPUT> result)
{
    float2 pos0 = input[0].m_position.xy / input[0].m_position.w;
    float2 pos1 = input[1].m_position.xy / input[1].m_position.w;

    float2 dir = pos0 - pos1;
    dir = normalize(float2(dir.x, dir.y * gGlobals.mCamera.mViewport.y / gGlobals.mCamera.mViewport.x)); // correct for aspect ratio
    float2 tng0 = float2(-dir.y, dir.x);
    float2 tng1 = tng0 * input[1].m_size / gGlobals.mCamera.mViewport;
    tng0 = tng0 * input[0].m_size / gGlobals.mCamera.mViewport;

    VS_OUTPUT ret_start;
    VS_OUTPUT ret_end;
    
    ret_start.m_size = input[0].m_size;
    ret_start.m_color = input[0].m_color;
    ret_start.m_uv = float2(0.0, 0.0);

    ret_end.m_size = input[1].m_size;
    ret_end.m_color = input[1].m_color;
    ret_end.m_uv = float2(1.0, 1.0);

    ret_start.m_position = float4((pos0 - tng0) * input[0].m_position.w, input[0].m_position.zw); 
    ret_start.m_edgeDistance = -input[0].m_size;
    result.Append(ret_start);
    
    ret_end.m_position = float4((pos1 - tng1) * input[1].m_position.w, input[1].m_position.zw);
    ret_end.m_edgeDistance = -input[1].m_size;
    result.Append(ret_end);

    ret_start.m_position = float4((pos0 + tng0) * input[0].m_position.w, input[0].m_position.zw);
    ret_start.m_edgeDistance = input[0].m_size;
    result.Append(ret_start);

    ret_end.m_position = float4((pos1 + tng1) * input[1].m_position.w, input[1].m_position.zw);
    ret_end.m_edgeDistance = input[1].m_size;
    result.Append(ret_end);
}
