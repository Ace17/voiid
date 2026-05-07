#version 310 es

precision mediump float;

// Uniforms
const int ToneMappingType = 0;

layout(binding = 0) uniform sampler2D InputTex1;
layout(binding = 1) uniform sampler2D InputTex2;

// Interpolated values from the vertex shader
layout(location = 0) in vec2 UV;

// Ouput data
layout(location = 0) out vec4 color;

vec3 Tonemap_ACES(vec3 x)
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return (x * (a * x + b)) / (x * (c * x + d) + e);
}

vec3 Tonemap_Reinhard(vec3 hdrColor)
{
  return hdrColor / (hdrColor + vec3(1.0));
}

void main()
{
  const float gamma = 1.0;
  vec3 hdrColor = texture(InputTex1, UV).rgb + texture(InputTex2, UV).rgb;

  vec3 mapped;

  if(ToneMappingType == 1)
    mapped = Tonemap_ACES(hdrColor);
  else
    mapped = Tonemap_Reinhard(hdrColor);

  // gamma correction
  mapped = pow(mapped, vec3(1.0 / gamma));

  color = vec4(mapped, 1.0);
}

// vim: syntax=glsl
