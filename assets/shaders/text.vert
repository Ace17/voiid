#version 310 es

// Input vertex data, different for all executions of this shader
layout(location = 0) in vec4 vertexPos_model;
layout(location = 1) in vec2 vertexUV;

// Output data; will be interpolated for each fragment
layout(location = 0) out vec2 UV;

// Values that stay constant for the whole mesh.
layout(location = 0) uniform mat4 MVP;

void main()
{
  gl_Position = MVP * vertexPos_model;
  UV = vertexUV;
}
// vim: syntax=glsl
