#version 300 es

// Input vertex data, different for all executions of this shader
in vec4 vertexPos_model;
in vec2 vertexUV;

// Output data; will be interpolated for each fragment
out vec2 UV;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;

void main()
{
  gl_Position = MVP * vertexPos_model;
  UV = vertexUV;
}
// vim: syntax=glsl
