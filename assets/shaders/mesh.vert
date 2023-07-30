#version 310 es

// Uniforms
layout(std140, binding=0) uniform MyUniformBlock
{
  mat4 M;
  mat4 MVP;
  vec4 fragOffset;
  vec3 CameraPos;
  vec3 ambientLight;
  vec3 LightPos[32];
  vec3 LightColor[32];
  int LightCount;
};

// Input Vertex Attributes
layout(location = 0) in vec4 vertexPos_model;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec2 vertexUV_lightmap;
layout(location = 3) in vec3 a_normal;
layout(location = 4) in vec3 a_binormal;
layout(location = 5) in vec3 a_tangent;

// Output Vertex Attributes
layout(location = 0) out vec2 UV;
layout(location = 1) out vec2 UV_lightmap;
layout(location = 2) out vec3 vPos;
layout(location = 3) out mat3 TBN;

void main()
{
  gl_Position = MVP * vertexPos_model;
  UV = vertexUV;
  UV_lightmap = vertexUV_lightmap;

  vPos = (M * vertexPos_model).xyz;

  // create tangent-space matrix
  vec3 T = normalize(M * vec4(a_tangent, 0)).xyz;
  vec3 B = normalize(M * vec4(a_binormal, 0)).xyz;
  vec3 N = normalize(M * vec4(a_normal, 0)).xyz;
  TBN = mat3(T, B, N);
}
// vim: syntax=glsl
