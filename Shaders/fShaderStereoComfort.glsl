#version 400

in vec4 eyePosition;

out vec4 finalColor;

void main(void)
{
  vec4 green = vec4(0.0, 1.0, 0.0, 0.5);
  vec4 red = vec4(1.0, 0.0, 0.0, 0.5);
  vec4 orange = vec4(1.0, 0.5, 0.0, 0.5);
  
  float z = abs(eyePosition.z);
  float alpha;
  
  if (z > 0.75)
  {
    alpha = (z-0.75)/0.25;
    finalColor = mix(orange, red, alpha);
  }
  else
  {
    finalColor = mix(green, orange, z/0.75);
  }
  
  //finalColor = red;
}