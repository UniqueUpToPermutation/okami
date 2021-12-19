struct VSInput {
    float3 Pos  : ATTRIB0;
    float2 UV   : ATTRIB1;
};

struct PSInput { 
    float4 Pos   : SV_POSITION;
    float2 UV    : TEXCOORD0;
};

void main(in VSInput vs_input,
          out PSInput vs_output) 
{
    vs_output.Pos    = float4(vs_input.Pos, 1.0);
    vs_output.UV     = vs_input.UV;
}