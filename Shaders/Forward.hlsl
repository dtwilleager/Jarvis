//***************************************************************************************
// Forward.hlsl by Doug Twilleager (C) 2018 All Rights Reserved.
//
// Basic Forward Shader
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

struct VertexIn
{
	float3 Pos    : POSITION;
  float3 Normal : NORMAL;
  float2 TexC   : TEXCOORD;
  float3 Tangent: TANGENT;
};

struct VertexOut
{
	float4 Pos    : SV_POSITION;
  float3 Normal : NORMAL;
  float2 TexC   : TEXCOORD;
  float3 Tangent: TANGENT;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.Pos = mul(float4(vin.Pos, 1.0f), gWorldViewProj);
	
  vout.Normal = vin.Normal;
  vout.TexC = vin.TexC;
  vout.Tangent = vin.Tangent;

  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  return float4(0.6, 0.6, 0.6, 1.0);
}


