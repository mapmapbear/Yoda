#define M_PI 3.14159265359


cbuffer CB : register(b0)
{
    float4x4 transformMat;
    float4 projectionVec;
};


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

Texture2D t_Texture : register(t0);
SamplerState s_Sampler : register(s0);

float4 main_ps(VertexOut pin) : SV_Target
{
    float3 cube_normal;
    cube_normal.z = -1.0;
    cube_normal.x = (cube_normal.z * (pin.uv.x)) / projectionVec.y;
	cube_normal.y = (cube_normal.z * (-pin.uv.y)) / projectionVec.w;
    cube_normal = mul(float4(cube_normal, 1.0), transformMat).xyz;
    cube_normal = normalize(cube_normal);
    float2 uv = pin.uv * 0.5 + 0.5;
    float2 panorama_coords = float2(atan2(cube_normal.x, -cube_normal.z), acos(cube_normal.y));

    if (panorama_coords.x < 0.0) {  
		panorama_coords.x += M_PI * 2.0;
	}

	panorama_coords /= float2(M_PI * 2.0, M_PI);

    float4 o_color = t_Texture.Sample(s_Sampler, panorama_coords);
    return o_color;
}
