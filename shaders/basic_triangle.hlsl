struct VertexIn
{
    float3 PosL : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
	float4 PosW : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};
struct ConstantBufferBlock
{
	float4x4 worldMat;
	float4x4 viewProj;
	float4 cameraPos;
	float4 LightDir;
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
Texture2D t_Normal : register(t1);
Texture2D t_RMO : register(t2);
Texture2DArray t_irradiacneTextArray : register(t3);
Texture2DArray t_specularTextArray : register(t4);
SamplerState s_MaterialSampler : register(s0);



//------------------BRDF-------------------
#define PI 3.1415926 
struct BRDFContext
{
	float NdotV;
	float NdotL;
	float NdotH;
	float VdotH;
	float LdotH;
};

void InitBRDF(float3 L, float3 V, float3 N, out BRDFContext context)
{
	float3 H = normalize(L + V);
	context.LdotH = saturate(dot(L, H));
	context.NdotH = saturate(dot(N, H));
	context.NdotV = abs(dot(N, V)) + 1e-5f;	// This abs allow to avoid artifact
	context.NdotL = saturate(dot(N, L));
	context.VdotH = saturate(dot(V, H));
}

float D_GGX(float NoH, float roughness) {
    float m2 = roughness * roughness;
	float f = (NoH * m2 - NoH) * NoH + 1.0f;
    return m2 / (f * f);
}
float pow5(float x)
{
	float x2 = x * x;
	return x2 * x2 * x;
}

float3 F_Schlick(float VoH, float3 f0, float3 f90) {
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float V_SmithGGX(float NoV, float NoL, float roughness)
{
	float a2 = roughness * roughness;
    float G_SmithGGX1V = NoV + sqrt((-NoV * a2 + NoV) * NoV + a2);
	float G_SmithGGX1L = NoL + sqrt((-NoL * a2 + NoL) * NoL + a2);
	
	return 1.0f / (G_SmithGGX1V * G_SmithGGX1L);
}

float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(NoL, 1.0, f90).r;
    float viewScatter = F_Schlick(NoV, 1.0, f90).r;
    return lightScatter * viewScatter * (1.0 / PI);
}

float Fr_Disney(float NoV, float NoL, float LoH, float linearRoughness, float roughness)
{
	float energyBias = lerp(0.0f, 0.5f, linearRoughness);
	float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
	float fd90 = energyBias + 2.0f * LoH*LoH * linearRoughness;
	float3 f0 = float3(1.0f, 1.0f, 1.0f);
	float lightScatter = F_Schlick(NoL, f0, fd90).r;
	float viewScatter = F_Schlick(NoV, f0, fd90).r;
	return lightScatter * viewScatter * energyFactor;
}

float linearRoughnessToRoughness(float roughness)
{
	return roughness * roughness;
}

float3 BRDF(BRDFContext data, float3 albedo, float roughness, float metallic)
{
	float NoH = data.NdotH;
	float VoH = data.VdotH;
	float NoV = data.NdotV;
	float NoL = data.NdotL; 
	float LoH = data.LdotH;
	float linearRoughness = linearRoughnessToRoughness(roughness);

	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, albedo, metallic);

	float3 F = F_Schlick(LoH, F0, 1.0);
	float D = D_GGX(NoH, linearRoughness);
	float V = V_SmithGGX(NoV, NoL, linearRoughness);

	float3 Fr = (D * V) * F;
	float3 Fd = Fr_Disney(NoV, NoL, LoH, linearRoughness, roughness);
	return Fd;
}
//-----------------------------------------
VertexOut main_vs(
	uint i_vertexId : SV_VertexID,
	VertexIn vin)
{
	VertexOut vout;
	vout.PosH = mul(g_camera.viewProj, float4(vin.PosL, 1.0));
	vout.PosW = mul(g_camera.worldMat, float4(vin.PosL, 1.0));
	vout.uv = vin.uv;
	vout.normal = mul(g_camera.worldMat, float4(vin.normal, 1.0)).xyz;
	return vout;
}



float4 main_ps(VertexOut pin) : SV_Target
{
    float4 o_color = t_Texture.Sample(s_MaterialSampler, pin.uv);
	BRDFContext context;
	float3 L = normalize(g_camera.LightDir.xyz);
	float3 V = normalize(g_camera.cameraPos.xyz - pin.PosW.xyz);
	float3 N = normalize(pin.normal);
	InitBRDF(L, V, N, context);
	// o_color += t_Normal.Sample(s_MaterialSampler, pin.uv) * 0.02;
	// o_color += t_RMO.Sample(s_MaterialSampler, pin.uv) * 0.02;
	// o_color += t_irradiacneTextArray.Sample(s_MaterialSampler, float3(pin.uv, 0.0)) * 0.2;
	// o_color += t_specularTextArray.Sample(s_MaterialSampler, float3(pin.uv, 0.0)) * 0.2;
	float3 lighting = BRDF(context, o_color.xyz, 0.02, 0.2);
	// o_color.xyz = pow(o_color.xyz, 1.0 / 2.2);
	o_color.xyz = lighting;
    return o_color;
}
