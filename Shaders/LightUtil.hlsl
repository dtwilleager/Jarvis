//***************************************************************************************
// LightUtil.hlsl by Doug Twilleager (C) 2018 All Rights Reserved.
//
// Lighting Utilities for PBR based lighting
//***************************************************************************************

#define PI 3.14159265359f

float3 Fresnel_Schlick(float3 specularColor, float3 l, float3 h)
{
  return (specularColor + (1.0f - specularColor) * pow((1.0f - dot(l, h)), 5));
}

float3 Specular_F(float3 specularColor, float3 l, float3 h)
{
  //return Fresnel_None(specularColor);
  return Fresnel_Schlick(specularColor, l, h);
  //return Fresnel_CookTorrance(specularColor, h, v);
}

float NormalDistribution_GGX(float a, float NdH)
{
  // Isotropic ggx.
  float a2 = a * a;
  float NdH2 = NdH * NdH;

  float denominator = NdH2 * (a2 - 1.0f) + 1.0f;
  denominator *= denominator;
  denominator *= PI;

  return a2 / denominator;
}

float NormalDistribution_BlinnPhong(float a, float NdH)
{
  return (1 / (PI * a * a)) * pow(NdH, 2 / (a * a) - 2);
}

float NormalDistribution_Beckmann(float a, float NdH)
{
  float a2 = a * a;
  float NdH2 = NdH * NdH;

  return (1.0f / (PI * a2 * NdH2 * NdH2 + 0.001)) * exp((NdH2 - 1.0f) / (a2 * NdH2));
}

float Specular_D(float a, float NdH)
{
  //return NormalDistribution_BlinnPhong(a, NdH);
  //return NormalDistribution_Beckmann(a, NdH);
  return NormalDistribution_GGX(a, NdH);
}

//-------------------------- Geometric shadowing -------------------------------------------
float Geometric_Implicit(float a, float NdV, float NdL)
{
  return NdL * NdV;
}

float Geometric_Neumann(float a, float NdV, float NdL)
{
  return (NdL * NdV) / max(NdL, NdV);
}

float Geometric_CookTorrance(float a, float NdV, float NdL, float NdH, float VdH)
{
  return min(1.0f, min((2.0f * NdH * NdV) / VdH, (2.0f * NdH * NdL) / VdH));
}

float Geometric_Kelemen(float a, float NdV, float NdL, float LdV)
{
  return (2 * NdL * NdV) / (1 + LdV);
}

float Geometric_Beckman(float a, float dotValue)
{
  float c = dotValue / (a * sqrt(1.0f - dotValue * dotValue));

  if (c >= 1.6f)
  {
    return 1.0f;
  }
  else
  {
    float c2 = c * c;
    return (3.535f * c + 2.181f * c2) / (1 + 2.276f * c + 2.577f * c2);
  }
}

float Geometric_Smith_Beckmann(float a, float NdV, float NdL)
{
  return Geometric_Beckman(a, NdV) * Geometric_Beckman(a, NdL);
}

float Geometric_GGX(float a, float dotValue)
{
  float a2 = a * a;
  return (2.0f * dotValue) / (dotValue + sqrt(a2 + ((1.0f - a2) * (dotValue * dotValue))));
}

float Geometric_Smith_GGX(float a, float NdV, float NdL)
{
  return Geometric_GGX(a, NdV) * Geometric_GGX(a, NdL);
}

float Geometric_Smith_Schlick_GGX(float a, float NdV, float NdL)
{
  // Smith schlick-GGX.
  float k = a * 0.5f;
  float GV = NdV / (NdV * (1 - k) + k);
  float GL = NdL / (NdL * (1 - k) + k);

  return GV * GL;
}

float Specular_G(float a, float NdV, float NdL, float NdH, float VdH, float LdV)
{
  //return Geometric_Implicit(a, NdV, NdL);
  //return Geometric_Neumann(a, NdV, NdL);
  //return Geometric_CookTorrance(a, NdV, NdL, NdH, VdH);
  //return Geometric_Kelemen(a, NdV, NdL, LdV);
  //return Geometric_Smith_Beckmann(a, NdV, NdL);
  return Geometric_Smith_GGX(a, NdV, NdL);
  //return Geometric_Smith_Schlick_GGX(a, NdV, NdL);
}

float3 Specular(float3 specularColor, float3 h, float3 v, float3 l, float a, float NdL, float NdV, float NdH, float VdH, float LdV)
{
  float3 fresnel = Specular_F(specularColor, l, h);
  float normalDistribution = Specular_D(a, NdH);
  float visibility = Specular_G(a, NdV, NdL, NdH, VdH, LdV) / (4.0f * NdL * NdV + 0.0001f);
  return (fresnel * normalDistribution * visibility);
}

