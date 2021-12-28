#include "Types.hlsl"

[maxvertexcount(4)]
void main(point SpriteBatchGSInput gs_input[1], 
	inout TriangleStream<SpriteBatchPSInput> triStream)
{
    SpriteBatchPSInput v;
    v.mColor = gs_input[0].mColor;

	float2 uvTL = gs_input[0].mUVTop;
	float2 uvBR = gs_input[0].mUVBottom;
	float2 uvTR = float2(uvBR.x, uvTL.y);
	float2 uvBL = float2(uvTL.x, uvBR.y);

	float4 left = gs_input[0].mPos - gs_input[0].mUVX;
	float4 right = gs_input[0].mPos + gs_input[0].mUVX;
	float4 pTL = left + gs_input[0].mUVY;
	float4 pTR = right + gs_input[0].mUVY;
	float4 pBL = left - gs_input[0].mUVY;
	float4 pBR = right - gs_input[0].mUVY;
 
    // create sprite quad
    // bottom left
	v.mPos = pBL;
	v.mUV = uvBL;
    triStream.Append(v);
 
    // top left
    v.mPos = pTL;
	v.mUV = uvTL;
    triStream.Append(v);
 
    // bottom right
	v.mPos = pBR;
	v.mUV = uvBR;
    triStream.Append(v);
 
    // top right
	v.mPos = pTR;
	v.mUV = uvTR;
    triStream.Append(v);
}