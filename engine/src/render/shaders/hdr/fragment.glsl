#version 300 es

precision mediump float;

// Interpolated values from the vertex shader
in vec2 UV;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh
uniform sampler2D HdrTex;

void main()
{
  const float gamma = 1.2;
  vec3 hdrColor = texture(HdrTex, UV).rgb;

  // reinhard tone mapping
  vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

  // gamma correction
  mapped = pow(mapped, vec3(1.0 / gamma));

  color = vec4(mapped, 1.0);
}

// vim: syntax=glsl
