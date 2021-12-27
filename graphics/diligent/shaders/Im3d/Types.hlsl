#define kAntialiasing 2.0

struct VS_OUTPUT
{
	float4 m_position     : SV_POSITION;
	float4 m_color        : COLOR;
	float2 m_uv           : TEXCOORD;
	float  m_size         : SIZE;
	float  m_edgeDistance : EDGE_DISTANCE;
};