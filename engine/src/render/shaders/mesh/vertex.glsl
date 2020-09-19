#version 300 es

// Input vertex data, different for all executions of this shader
in vec4 vertexPos_model;
in vec2 vertexUV;
in vec2 vertexUV_lightmap;
in vec3 a_normal;

// Output data; will be interpolated for each fragment
out vec2 UV;
out vec2 UV_lightmap;
out vec3 vPos;
out vec3 vNormal;

// Values that stay constant for the whole mesh.
uniform mat4 M;
uniform mat4 MVP;

void main()
{
  gl_Position = MVP * vertexPos_model;
  UV = vertexUV;
  UV_lightmap = vertexUV_lightmap;

  vPos = (M * vertexPos_model).xyz;
  vNormal = normalize((M * vec4(a_normal, 0)).xyz);
}
// vim: syntax=glsl
