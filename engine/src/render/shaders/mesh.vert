#version 310 es

// Input vertex data, different for all executions of this shader
layout(location = 0) in vec4 vertexPos_model;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec2 vertexUV_lightmap;
layout(location = 3) in vec3 a_normal;

// Output data; will be interpolated for each fragment
layout(location = 0) out vec2 UV;
layout(location = 1) out vec2 UV_lightmap;
layout(location = 2) out vec3 vPos;
layout(location = 3) out vec3 vNormal;

// Values that stay constant for the whole mesh.
layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat4 MVP;

void main()
{
  gl_Position = MVP * vertexPos_model;
  UV = vertexUV;
  UV_lightmap = vertexUV_lightmap;

  vPos = (M * vertexPos_model).xyz;
  vNormal = normalize((M * vec4(a_normal, 0)).xyz);
}
// vim: syntax=glsl
