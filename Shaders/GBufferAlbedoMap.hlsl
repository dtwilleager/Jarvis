//***************************************************************************************
// Forward.hlsl by Doug Twilleager (C) 2018 All Rights Reserved.
//
// Basic Forward Shader
//***************************************************************************************

#include "LightUtil.hlsl"

Texture2D g_albedotexture : register(t0);
SamplerState g_pointClampSampler : register(s0);
SamplerState g_pointWrapSampler : register(s1);
SamplerState g_linearClampSampler : register(s2);
SamplerState g_linearWrapSampler : register(s3);

struct Light
{
  float3 color;
  float pad1;
  float3 direction;
  float pad2;
};

cbuffer cbFrame : register(b2)
{
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProj;
  float4x4 gInvProj;
  float4x4 gViewProj;
  float4x4 gInvViewProj;
  float4 gAmbientLight;
  float3 gEyePosW;
  uint gNumLights;
  Light gLights[64];
  float2 gRenderTargetSize;
  float2 gInvRenderTargetSize;
  float gNearZ;
  float gFarZ;
};

cbuffer cbPerMaterial : register(b1)
{
  float4 gDiffuseAlbedo;
  float3 gFresnelR0;
  float  gRoughness;
  float4x4 gTextureTransform;
};

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld; 
  float4x4 gModel;
};

struct VertexIn
{
  float3 Pos    : POSITION;
  float3 Normal : NORMAL;
  float3 TexC   : TEXCOORD;
  float3 Tangent: TANGENT;
};

struct VertexOut
{
	float4 Pos      : SV_POSITION;
  float3 WPos     : POSITION;
  float3 TexC     : TEXCOORD;
  float3 Normal   : NORMAL;
};

struct PsOutput
{
  float3 albedo : SV_TARGET0;
  float4 normal : SV_TARGET1;
  float4 position : SV_TARGET2;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
  vout.WPos = mul(float4(vin.Pos, 1.0f), gWorld).xyz;
  vout.Pos = mul(float4(vout.WPos, 1.0f), gViewProj);
  vout.TexC = vin.TexC;

  vout.Normal = normalize(mul(vin.Normal, (float3x3)gWorld));

  return vout;
}

PsOutput PS(VertexOut pin)
{
  PsOutput output;

  float4 albedo = g_albedotexture.Sample(g_linearWrapSampler, pin.TexC.xy);

  if (albedo.a < 1.0)
    discard;

  output.albedo = albedo.rgb;
  output.normal = float4(pin.Normal, gRoughness);
  output.position = float4(pin.WPos, 0.0);

  return output;
}


