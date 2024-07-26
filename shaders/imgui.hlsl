struct Constants
{
    float2 invDisplaySize;
};

cbuffer g_Const : register(b0) { Constants g_Const; }
sampler sampler0 : register(s0);
Texture2D texture0 : register(t0);

struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

struct PS_OUTPUT
{
    float4 out_pos : SV_POSITION;
    float4 out_col : COLOR0;
    float2 out_uv  : TEXCOORD0;
};

PS_OUTPUT main_vs(VS_INPUT input)
{
    PS_OUTPUT output;
    output.out_pos.xy = input.pos.xy * g_Const.invDisplaySize * float2(2.0, -2.0) + float2(-1.0, 1.0);
    output.out_pos.zw = float2(0, 1);
    output.out_col = input.col;
    output.out_uv = input.uv;
    return output;
}


float4 main_ps(PS_OUTPUT input) : SV_Target
{
    float4 out_col = input.out_col * texture0.Sample(sampler0, input.out_uv);
    return out_col;
}
