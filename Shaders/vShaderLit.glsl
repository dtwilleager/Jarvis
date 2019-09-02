#version 400

layout(location=0) in vec4 in_position;
layout(location=1) in vec3 in_normal;
layout(location=2) in vec2 in_textureCoordinate;
//layout(location=3) in mat4 in_modelViewMatrix;
out vec3 ex_normal;
out vec2 ex_textureCoordinate;
out vec4 eyePosition;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

void main(void)
{
  mat3 normalMatrix = mat3(modelViewMatrix);
  gl_Position = projectionMatrix * modelViewMatrix * in_position;
  eyePosition = modelViewMatrix * in_position;
  ex_normal = normalMatrix * in_normal;
  ex_textureCoordinate = in_textureCoordinate;
}