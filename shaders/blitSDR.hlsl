struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 uv : TEXCOORD;
};


VertexOut main_vs(uint i_vertexId : SV_VertexID)
{
    VertexOut vout;
    float2 base_arr[3] = {float2(-1.0, -3.0), float2(-1.0, 1.0), float2(3.0, 1.0)};
    vout.uv = base_arr[i_vertexId];
    vout.PosH = float4(vout.uv, 0, 1);
    return vout;
}

float3 ApplyDisplaySRGB(float3 color)
{
    return pow(color, 1.0 / 2.2);
}

Texture2D t_Texture : register(t0);

float4 main_ps(VertexOut pin) : SV_Target
{
    float3 MainColor = ApplyDisplaySRGB(t_Texture[(int2)pin.PosH.xy]);
    return float4(MainColor, 1.0);
}