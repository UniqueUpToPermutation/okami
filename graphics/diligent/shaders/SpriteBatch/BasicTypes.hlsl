#ifndef BASIC_SPRITE_BATCH_TYPES_HLSL_
#define BASIC_SPRITE_BATCH_TYPES_HLSL_

struct SpriteBatchVSInput
{
	// Use W component as rotation
    float4 mPos     	: ATTRIB0;
	float4 mColor 		: ATTRIB1;

	float2 mUVTop 		: ATTRIB2;
	float2 mUVBottom 	: ATTRIB3;

	float2 mSize 		: ATTRIB4;
	float2 mOrigin		: ATTRIB5;
};

struct SpriteBatchGSInput
{
	float4 mPos  		: SV_POSITION;
	float4 mColor		: COLOR0;
	float4 mUVX			: TEXCOORD0;
	float4 mUVY			: TEXCOORD1;
	float2 mUVTop		: TEXCOORD2;
	float2 mUVBottom	: TEXCOORD3;
};

struct SpriteBatchPSInput 
{
	float4 mPos 		: SV_POSITION;
	float4 mColor		: COLOR0;
	float2 mUV			: TEXCOORD0;
};

#endif