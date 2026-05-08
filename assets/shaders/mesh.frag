#version 310 es

precision mediump float;

// Uniforms
struct ShaderLight
{
  vec3 pos;
  float range;
  vec3 color;
  float pad2;
};

layout(binding=0, std140) uniform MyUniformBlock
{
  mat4 M;
  mat4 MVP;
  vec4 fragOffset;
  vec3 CameraPos;
  vec3 ambientLight;
  ShaderLight Lights[32];
  int LightCount;
};

layout(binding = 1) uniform sampler2D DiffuseTex;
layout(binding = 2) uniform sampler2D NormalTex;
layout(binding = 3) uniform sampler2D EmissiveTex;

// Input Vertex Attributes
layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 vPos;
layout(location = 2) in mat3 TBN;

// Ouput data
layout(location = 0) out vec4 color;

float sqr(float x)
{
  return x * x;
}

void main()
{
  vec3 totalLight = vec3(0, 0, 0);
  vec4 diffuse = texture(DiffuseTex, UV);

  // highlight
  totalLight += fragOffset.rgb;

  // ambient
  totalLight += ambientLight * diffuse.rgb;

  // emissive
  totalLight += texture(EmissiveTex, UV).rgb * 5.0;

  vec3 localN = texture(NormalTex, UV).rgb * 2.0 - 1.0;
  vec3 normal = normalize(TBN * localN);

  // dynamic lights
  for(int i=0;i < LightCount;++i)
  {
    vec3 lightDir = normalize(Lights[i].pos - vPos);

    // https://lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html
    float s = clamp(length(Lights[i].pos - vPos)/Lights[i].range, 0.0, 1.0);
    float F = 16.0;
    float attenuation = sqr(1.0 - sqr(s))/(1.0 + F * sqr(s));

    float incidenceRatio = max(0.0, dot(lightDir, normal));

    // diffuse
    totalLight += Lights[i].color * incidenceRatio * attenuation * diffuse.rgb * diffuse.a;

    // specular
    const float material_shininess = 256.0;
    const float material_specular = 0.01;
    vec3 viewDir = normalize(CameraPos - vPos);
    vec3 halfwayDir = normalize((viewDir + lightDir) * 0.5);
    float angle = max(dot(normal, halfwayDir), 0.0);
    float spec = pow(angle, material_shininess);
    totalLight += Lights[i].color * (spec * material_specular);
  }

  color.rgb = totalLight;
  color.a = diffuse.a;

  if(false)
  {
    color.rgb = texture(NormalTex, UV).rgb;
    color.a = 1.0;
  }
}

// vim: syntax=glsl
