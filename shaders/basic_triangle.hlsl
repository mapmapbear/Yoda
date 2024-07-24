struct VertexIn
{
    float3 PosL : POSITION;
    // float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    // float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};
struct ConstantBufferBlock
{
	float4x4 viewProj;
};
ConstantBuffer<ConstantBufferBlock> g_camera : register(b0);

static const float2 g_positions[] = {
	float2(-0.5, -0.5),
	float2(0, 0.5),
	float2(0.5, -0.5)
};

static const float3 g_colors[] = {
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1)	
};

Texture2D t_Texture : register(t0);
SamplerState s_MaterialSampler : register(s0);

VertexOut main_vs(
	uint i_vertexId : SV_VertexID,
	VertexIn vin)
{
	VertexOut vout;
	vout.PosH = mul(g_camera.viewProj, float4(vin.PosL, 1.0));
	vout.uv = vin.uv;
	// vout.PosH = mul(g_camera.viewProj, float4(vin.PosL, 1.0));
	return vout;
}

float4 main_ps(VertexOut pin) : SV_Target
{
    float4 o_color = t_Texture.Sample(s_MaterialSampler, pin.uv);
    return o_color;
}
