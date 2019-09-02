#version 400
out vec4 finalColor;

uniform vec3 diffuseColor;

void main(void)
{
  finalColor = vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, 1);
}