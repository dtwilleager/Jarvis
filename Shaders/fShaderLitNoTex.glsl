#version 400
in vec3 ex_normal;
in vec4 eyePosition;
out vec4 finalColor;

struct Light
{
  vec3 position;
  vec3 direction;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 attenuation;
};

uniform Light lights[4];
uniform int numLights;

uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 ambientColor;
uniform vec3 emissiveColor;
uniform vec3 transparentColor;
uniform float shininess;

void main(void)
{
  vec3 normal = normalize(ex_normal);
  vec3 lightDir;
  vec3 halfVector;
  vec3 color;
  float NdotL;
  float NdotHV;
  vec3 eye;
  
  eye.x = eyePosition.x; eye.y = eyePosition.y; eye.z = eyePosition.z; 
  color = diffuseColor*ambientColor;
  
  for (int i=0; i<numLights; i++)
  {
    lightDir = normalize(lights[i].position - eye);
    halfVector = normalize(lightDir + vec3(0, 0, 1));
    NdotL = max(dot(normal,lightDir),0.0);
    if (NdotL > 0.0) 
    {
      color += diffuseColor * NdotL * lights[i].diffuse;
      NdotHV = max(dot(normal,halfVector),0.0);
      color += specularColor * lights[i].specular * pow(NdotHV, 32);
    }
  }
 
  finalColor = vec4(color.r, color.g, color.b, 1);
}