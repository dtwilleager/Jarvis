//***************************************************************************************
// Forward.hlsl by Doug Twilleager (C) 2018 All Rights Reserved.
//
// Basic Forward Shader
//***************************************************************************************

#include "LightUtil.hlsl"

Texture2D g_albedotexture : register(t0);
Texture2D g_normaltexture : register(t1);
Texture2D g_roughnesstexture : register(t2);
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
	float4 Pos      : SV_POSITION;
  float3 WPos     : POSITION;
  float2 TexC     : TEXCOORD;
  float3 Normal   : NORMAL;
  float3 Tangent  : TANGENT;
  float3 Binormal : BINORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
  vout.WPos = mul(float4(vin.Pos, 1.0f), gWorld).xyz;
  vout.Pos = mul(float4(vout.WPos, 1.0f), gViewProj);
  vout.TexC = vin.TexC;

  //vout.Normal = vin.Normal;
  vout.Normal = normalize(mul(vin.Normal, (float3x3)gWorld));
  vout.Tangent = normalize(mul(vin.Tangent, (float3x3)gWorld));
  vout.Binormal = normalize(mul(cross(vin.Normal, vin.Tangent), (float3x3)gWorld));

  return vout;
}

float4 PS(VertexOut pin)
{
  float4 albedo = g_albedotexture.Sample(g_linearWrapSampler, pin.TexC);

  if (albedo.a < 1.0)
    discard;

  float3 normalSample = g_normaltexture.Sample(g_linearWrapSampler, pin.TexC).xyz;
  float roughness = g_roughnesstexture.Sample(g_linearWrapSampler, pin.TexC).x;
  float3 specularColor = float3(0.04, 0.04, 0.04);

  normalSample = normalize((normalSample * 2.0f) - 1.0f);
  float3x3 TBN = float3x3(normalize(pin.Tangent), normalize(pin.Binormal), normalize(pin.Normal));
  float3 normal = normalize(mul(normalSample, TBN));
  float3 viewDirection = normalize(gEyePosW - pin.WPos);
  float3 diffuse = albedo.xyz / PI;

  float3 finalColor = gAmbientLight.xyz;

  uint i;
  float intensity = 0.0;
  for (i = 0; i < gNumLights; i++)
  {
    float3 lightDirection = gLights[i].direction - pin.WPos;
    float lightDistance = length(lightDirection);
    lightDirection = normalize(lightDirection);
    float attenuation = 100 * PI / (lightDistance * lightDistance);

    float NdL = clamp(dot(normal, lightDirection), 0.0, 1.0);
    float NdV = clamp(dot(normal, viewDirection), 0.0, 1.0);
    float3 h = normalize(lightDirection + viewDirection);
    float NdH = clamp(dot(normal, h), 0.0, 1.0);
    float VdH = clamp(dot(viewDirection, h), 0.0, 1.0);
    float LdV = clamp(dot(lightDirection, viewDirection), 0.0, 1.0);
    float a = max(0.001f, roughness * roughness);

    float3 spec = Specular(specularColor, h, viewDirection, lightDirection, a, NdL, NdV, NdH, VdH, LdV);

    float shadow = 1.0; //computeShadow(i, world_position);

    finalColor += (attenuation * shadow * NdL * gLights[i].color * (diffuse * (1.0 - spec) + spec));
  }

  float3 lightDirection = normalize(float3(-1.0, -1.0, -1.0));

  float NdL = clamp(dot(normal, lightDirection), 0.0, 1.0);
  float NdV = clamp(dot(normal, viewDirection), 0.0, 1.0);
  float3 h = normalize(lightDirection + viewDirection);
  float NdH = clamp(dot(normal, h), 0.0, 1.0);
  float VdH = clamp(dot(viewDirection, h), 0.0, 1.0);
  float LdV = clamp(dot(lightDirection, viewDirection), 0.0, 1.0);
  float a = max(0.001f, roughness * roughness);

  float3 spec = Specular(specularColor, h, viewDirection, lightDirection, a, NdL, NdV, NdH, VdH, LdV);

  float shadow = 1.0; //computeShadow(i, world_position);

  finalColor += (shadow * NdL * float3(1.0, 1.0, 1.0) * (diffuse * (1.0 - spec) + spec));

  return float4(finalColor, 1.0);
}


