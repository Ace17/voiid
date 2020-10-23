#version 310 es

// Attributes
layout(location = 0) in vec2 vertexPos_model;
layout(location = 1) in vec2 vertexUV;

// Output data; will be interpolated for each fragment
layout(location = 0) out vec2 UV;

void main()
{
  gl_Position = vec4(vertexPos_model, 1, 1);
  UV = vertexUV;
}
// vim: syntax=glsl
