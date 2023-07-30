#version 310 es

precision mediump float;

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

layout(binding = 0) uniform sampler2D DiffuseTex;
layout(binding = 1) uniform sampler2D LightmapTex;
layout(binding = 2) uniform sampler2D NormalTex;

// Input Vertex Attributes
layout(location = 0) in vec2 UV;
layout(location = 1) in vec2 UV_lightmap;
layout(location = 2) in vec3 vPos;
layout(location = 3) in mat3 TBN;

// Ouput data
layout(location = 0) out vec4 color;

void main()
{
  vec3 totalLight = vec3(0, 0, 0);

  // highlight
  totalLight += fragOffset.rgb;

  // ambient
  totalLight += vec3(1, 1, 1) * ambientLight * texture(DiffuseTex, UV).rgb;

  // lightmap
  totalLight += texture(LightmapTex, UV_lightmap).rgb * 0.01 * texture(DiffuseTex, UV).rgb;

  vec3 localN = texture(NormalTex, UV).rgb * 2.0 - 1.0;
  vec3 normal = TBN * localN;

  // dynamic lights
  for(int i=0;i < LightCount;++i)
  {
    vec3 lightDir = normalize(LightPos[i] - vPos);
    float lightDist = length(LightPos[i] - vPos);
    float attenuation = 10.0/(lightDist*lightDist*lightDist);
    float incidenceRatio = max(0.0, dot(lightDir, normal));

    // diffuse
    totalLight += LightColor[i] * incidenceRatio * attenuation * texture(DiffuseTex, UV).rgb;

    // specular
    const float material_shininess = 256.0;
    const float material_specular = 0.01;
    vec3 viewDir = normalize(CameraPos - vPos);
    vec3 halfwayDir = normalize((viewDir + lightDir) * 0.5);
    float angle = max(dot(normal, halfwayDir), 0.0);
    float spec = pow(angle, material_shininess);
    totalLight += LightColor[i] * (spec * material_specular);
  }

  color.rgb = totalLight;
  color.a = texture(DiffuseTex, UV).a;

  if(false)
  {
    color.rgb = texture(NormalTex, UV).rgb;
    color.a = 1.0;
  }
}

// vim: syntax=glsl
