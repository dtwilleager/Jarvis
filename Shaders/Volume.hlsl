//***************************************************************************************
// Forward.hlsl by Doug Twilleager (C) 2018 All Rights Reserved.
//
// Basic Forward Shader
//***************************************************************************************

#include "LightUtil.hlsl"

Texture3D g_albedotexture : register(t0);
Texture2D g_positiontexture  : register(t1);
SamplerState g_pointClampSampler : register(s0);
SamplerState g_pointWrapSampler : register(s1);
SamplerState g_linearClampSampler : register(s2);
SamplerState g_linearWrapSampler : register(s3);
SamplerState g_volumeSampler : register(s4);

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
};


VertexOut VS(VertexIn vin)
{
  VertexOut vout;

  vout.WPos = vin.Pos;
  float4 Wpos = mul(float4(vin.Pos, 1.0f), gWorld);
  //vout.WPos = mul(Wpos, gViewProj);
  //vout.WPos = mul(float4(vin.Pos, 1.0f), gWorld).xyz;
  vout.Pos = mul(float4(Wpos.xyz, 1.0f), gViewProj);

  vout.TexC = vin.TexC;

  return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
  //float finalValue = 0.0;
  //float3 texCoord = pin.TexC;
  //float3 oEyePos = mul(float4(gEyePosW, 1.0f), gModel).xyz;
  //float3 tcEyePos = mul(float4(oEyePos, 1.0f), gInvViewProj).xyz;
  ////float3 rayDirection = normalize(texCoord - tcEyePos);
  //float3 rayDirection = normalize(pin.WPos - oEyePos);
  ////float3 rayDirection = float3(0.0, 0.0, 1.0);
  //float maxValue = 0.0;
  //float scale = 1.0f / 100.0f;
  //float3 step = rayDirection * float3(scale, scale, scale);
  //float r = 0.0f;
  //float z = pin.Pos.z;
  //float inc = 0.0f;
  //bool g_exit = false;
  //int4 location;

  //while (!g_exit)
  //{
  //  float3 tc = texCoord + inc * step;
  //  location.x = (tc.x * 238.0f);
  //  location.y = (tc.y * 238.0f);
  //  location.z = (tc.z * 101.0f);
  //  location.w = 0;
  //  //float4 albedo = g_albedotexture.Sample(g_linearClampSampler, tc);
  //  float4 albedo = g_albedotexture.Load(location);
  //  float value = albedo.x* 10.0f;

  //  if (value > maxValue)
  //  {
  //    maxValue = value;
  //    r = location.z;
  //  }
  //  if (inc > 100.0f || maxValue > 0.99)
  //    break;

  //  inc += 1.0f;
  //  //texCoord += step;
  //}

  //if (maxValue < 0.01)
  //  discard;

  ////return float4(maxValue, maxValue, maxValue, 1.0);
  //return float4(r, r, r, 1.0);
  ////return float4(rayDirection.x, rayDirection.y, rayDirection.z, 1.0);

  ////float3 vPositionWS = g_positiontexture[pin.Pos.xy].xyz;

  ////return float4(albedo);

    float4 color = g_albedotexture.Sample(g_volumeSampler, pin.TexC);
    //float4 color = tex3d(g_sampler, pin.TexC);
    float value = color.x* 100.0f;

    if (value < 0.01 || value > 0.99)
      discard;

    return float4(value, value, value, 1.0);
}


