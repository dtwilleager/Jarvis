//***************************************************************************************
// Forward.hlsl by Doug Twilleager (C) 2018 All Rights Reserved.
//
// Basic Forward Shader
//***************************************************************************************

#include "LightUtil.hlsl"

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
  float3 Normal   : NORMAL;
};

VertexOut VS(VertexIn vin)
{
  VertexOut vout;

  vout.WPos = mul(float4(vin.Pos, 1.0f), gWorld).xyz;
  vout.Pos = mul(float4(vout.WPos, 1.0f), gViewProj);

  vout.Normal = normalize(mul(vin.Normal, (float3x3)gWorld));

  return vout;
}

float4 PS(VertexOut pin)
{
  float4 albedo = gDiffuseAlbedo;

  if (albedo.a < 1.0)
    discard;

  float3 specularColor = float3(0.04, 0.04, 0.04);

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

    float NdL = clamp(dot(pin.Normal, lightDirection), 0.0, 1.0);
    float NdV = clamp(dot(pin.Normal, viewDirection), 0.0, 1.0);
    float3 h = normalize(lightDirection + viewDirection);
    float NdH = clamp(dot(pin.Normal, h), 0.0, 1.0);
    float VdH = clamp(dot(viewDirection, h), 0.0, 1.0);
    float LdV = clamp(dot(lightDirection, viewDirection), 0.0, 1.0);
    float a = max(0.001f, gRoughness * gRoughness);

    float3 spec = Specular(specularColor, h, viewDirection, lightDirection, a, NdL, NdV, NdH, VdH, LdV);

    float shadow = 1.0; //computeShadow(i, world_position);

    finalColor += (attenuation * shadow * NdL * gLights[i].color * (diffuse * (1.0 - spec) + spec));
  }

  float3 lightDirection = normalize(float3(-1.0, -1.0, -1.0));

  float NdL = clamp(dot(pin.Normal, lightDirection), 0.0, 1.0);
  float NdV = clamp(dot(pin.Normal, viewDirection), 0.0, 1.0);
  float3 h = normalize(lightDirection + viewDirection);
  float NdH = clamp(dot(pin.Normal, h), 0.0, 1.0);
  float VdH = clamp(dot(viewDirection, h), 0.0, 1.0);
  float LdV = clamp(dot(lightDirection, viewDirection), 0.0, 1.0);
  float a = max(0.001f, gRoughness * gRoughness);

  float3 spec = Specular(specularColor, h, viewDirection, lightDirection, a, NdL, NdV, NdH, VdH, LdV);

  float shadow = 1.0; //computeShadow(i, world_position);

  finalColor += (shadow * NdL * float3(1.0, 1.0, 1.0) * (diffuse * (1.0 - spec) + spec));

  return float4(finalColor, 1.0);
}


