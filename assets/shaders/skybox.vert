#version 310 es

// Uniforms
layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat4 MVP;

// Attributes
layout(location = 0) in vec4 vertexPos_model;

// Output data; will be interpolated for each fragment
layout(location = 0) out vec3 vPos;

void main()
{
  vec4 pos = vertexPos_model;
  pos.xyz *= -1.0; // invert the box inside-out

  gl_Position = MVP * pos;

  vPos = vertexPos_model.xyz;
}

// vim: syntax=glsl
