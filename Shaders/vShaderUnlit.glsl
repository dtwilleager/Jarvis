#version 400

layout(location=0) in vec4 in_position;
layout(location=1) in mat4 in_modelViewMatrix;

uniform mat4 projectionMatrix;

void main(void)
{
  gl_Position = projectionMatrix * in_modelViewMatrix * in_position;
}