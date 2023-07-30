#version 310 es

// Uniforms
layout(std140, binding=0) uniform MyUniformBlock
{
  mat4 MVP;
};

// Attributes
layout(location = 0) in vec4 vertexPos_model;
layout(location = 1) in vec2 vertexUV;

// Output data; will be interpolated for each fragment
layout(location = 0) out vec2 UV;

void main()
{
  gl_Position = MVP * vertexPos_model;
  UV = vertexUV;
}
// vim: syntax=glsl
