//***************************************************************************************
// Forward.hlsl by Doug Twilleager (C) 2018 All Rights Reserved.
//
// Basic Forward Shader
//***************************************************************************************

#include "LightUtil.hlsl"

Texture2D g_albedotexture : register(t0);
Texture2D g_normaltexture : register(t1);
Texture2D g_positiontexture  : register(t2);
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
  float4 Pos    : POSITION;
  float2 TexC   : TEXCOORD;
};

struct VertexOut
{
  float4 Pos      : SV_POSITION;
  float2 TexC     : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
  vout.Pos = vin.Pos;
  vout.TexC = vin.TexC;

  return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
  float3 albedo = g_albedotexture[pin.Pos.xy].xyz;
  float4 normalSample = g_normaltexture[pin.Pos.xy];
  float3 normal = normalSample.xyz;
  float roughness = normalSample.w;
  float3 vPositionWS = g_positiontexture[pin.Pos.xy].xyz;

  float3 specularColor = float3(0.04, 0.04, 0.04);
  float3 viewDirection = normalize(gEyePosW - vPositionWS.xyz);
  float3 diffuse = albedo.xyz / PI;

  float3 finalColor = gAmbientLight.xyz;

  if (roughness < 0.0)
  {
    return float4(albedo.xyz, 1.0);
  }

  uint i;
  float intensity = 0.0;
  for (i = 0; i < gNumLights; i++)
  {
    float3 lightDirection = gLights[i].direction - vPositionWS.xyz;
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


