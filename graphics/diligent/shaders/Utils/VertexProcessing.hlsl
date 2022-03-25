#ifndef __NORMALS_HLSL__
#define __NORMALS_HLSL__

struct TransformedVertex
{
    float3 WorldPos;
    float3 Normal;
};

float3x3 InverseTranspose3x3(float3x3 M)
{
    // Note that in HLSL, M_t[0] is the first row, while in GLSL, it is the 
    // first column. Luckily, determinant and inverse matrix can be equally 
    // defined through both rows and columns.
    float det = dot(cross(M[0], M[1]), M[2]);
    float3x3 adjugate = float3x3(cross(M[1], M[2]),
                                 cross(M[2], M[0]),
                                 cross(M[0], M[1]));
    return adjugate / det;
}

TransformedVertex TransformVertex(in float3 Pos,
                                    in float3 Normal,
                                    in float4x4 Transform)
{
    TransformedVertex TransformedVert;
    
	float4 locPos = mul(Transform, float4(Pos, 1.0));
    float3x3 NormalTransform = float3x3(Transform[0].xyz, Transform[1].xyz, Transform[2].xyz);
    NormalTransform = InverseTranspose3x3(NormalTransform);
    Normal = mul(NormalTransform, Normal);
    float NormalLen = length(Normal);
    TransformedVert.Normal = Normal / max(NormalLen, 1e-5);

	TransformedVert.WorldPos = locPos.xyz / locPos.w;

    return TransformedVert;
}

#endif