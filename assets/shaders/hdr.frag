#version 310 es

precision mediump float;

// Interpolated values from the vertex shader
layout(location = 0) in vec2 UV;

// Ouput data
layout(location = 0) out vec4 color;

// Values that stay constant for the whole mesh
layout(location = 2) uniform sampler2D InputTex1;
layout(location = 3) uniform sampler2D InputTex2;

void main()
{
  const float gamma = 1.2;
  vec3 hdrColor = texture(InputTex1, UV).rgb + texture(InputTex2, UV).rgb;

  // reinhard tone mapping
  vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

  // gamma correction
  mapped = pow(mapped, vec3(1.0 / gamma));

  color = vec4(mapped, 1.0);
}

// vim: syntax=glsl
