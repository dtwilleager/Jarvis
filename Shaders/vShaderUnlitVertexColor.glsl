#version 400

layout(location=0) in vec4 in_position;
layout(location=1) in vec4 in_color;
layout(location=2) in mat4 in_modelViewMatrix;
out vec4 ex_color;

uniform mat4 projectionMatrix;

void main(void)
{
  ex_color = in_color;
  gl_Position = projectionMatrix * in_modelViewMatrix * in_position;
}