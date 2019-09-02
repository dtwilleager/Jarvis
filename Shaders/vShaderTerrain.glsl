#version 400

layout(location=0) in vec4 in_position;
layout(location=1) in vec3 in_normal;
layout(location=2) in mat4 in_modelViewMatrix;
out vec3 ex_normal;
out vec4 eyePosition;
out vec4 worldPosition;

//uniform mat4 modelViewMatrix;
//uniform mat3 normalMatrix;
uniform mat4 projectionMatrix;

void main(void)
{
  //mat3 normalMatrix = transpose(inverse(mat3(in_modelViewMatrix)));
  gl_Position = projectionMatrix * in_modelViewMatrix * in_position;
  worldPosition = in_position;
  eyePosition = in_modelViewMatrix * in_position;
  //ex_normal = normalMatrix * in_normal;
}