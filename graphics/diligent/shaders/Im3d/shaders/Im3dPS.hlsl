#include "Common.hlsl"
#include "../../Common.hlsl"

#if defined(SHADERED)
#line __LINE__ + 2
#endif

void main_lines(in GS_OUTPUT ps_input, out float4 result : SV_TARGET) {
	result = ps_input.m_color;
    float d = abs(ps_input.m_edgeDistance) / ps_input.m_size;
    d = smoothstep(1.0, 1.0 - (kAntialiasing / ps_input.m_size), d);

    result.a *= d;
}

void main_points(in GS_OUTPUT ps_input, out float4 result : SV_TARGET) {
	result = ps_input.m_color;
    float d = length(ps_input.m_uv - float2(0.5, 0.5));
    d = smoothstep(0.5, 0.5 - (kAntialiasing / ps_input.m_size), d);
    result.a *= d;
}

void main_tris(in VS_OUTPUT ps_input, out float4 result : SV_TARGET) {
	result = ps_input.m_color;
}