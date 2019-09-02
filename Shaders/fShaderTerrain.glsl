#version 400
in vec3 ex_normal;
in vec4 eyePosition;
in vec4 worldPosition;
out vec4 finalColor;

uniform sampler2D diffuseTexture;

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
  vec2 textureCoordinate;
  
  float height = worldPosition.y/7.0;
  
  textureCoordinate.x = worldPosition.x/51.2;
  textureCoordinate.y = worldPosition.z/51.2;
  
  vec3 diffuseTextureColor = vec3(texture2D(diffuseTexture, textureCoordinate));

  // 0.0 to .20 Blue, .20 - .25 lerp, .25 - .80 green, .80 - .85 lerp, .75 - 1.0 white
  
  if (height < 0.02)
  {
    color = vec3(0, 0, 0.5);
  }
  else if (height < 0.15)
  {
    color = mix(vec3(0, 0, 0.5), vec3(0, 0.5, 0), (height-0.02)/0.13);
  }
  else if (height < 0.65)
  {
    color = vec3(0, 0.5, 0);
  }
  else if (height < 0.75)
  {
    color = mix(vec3(0, 0.5, 0), vec3(1, 1, 1), (height-0.65)/0.10);
  }
  else
  {
    color = vec3(1, 1, 1);
  }
 
  color = diffuseTextureColor * color + vec3(0.2, 0.2, 0.2);
  finalColor = vec4(color.r, color.g, color.b, 1);
}