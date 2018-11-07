#version 300 es

// Input vertex data, different for all executions of this shader
in vec4 vertexPos_model;
in vec2 vertexUV;
in vec3 a_normal;

// Output data; will be interpolated for each fragment
out vec2 UV;
out vec3 vNormal;
out float fogFactor;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;

void main()
{
  gl_Position = MVP * vertexPos_model;
  UV = vertexUV;
  vNormal = normalize(a_normal);
  fogFactor = clamp(1.0/exp(length(gl_Position) * 0.01), 0.0, 1.0);
}
// vim: syntax=glsl
