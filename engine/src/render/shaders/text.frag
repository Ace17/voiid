#version 310 es

precision mediump float;

// Interpolated values from the vertex shader
layout(location = 0) in vec2 UV;

// Ouput data
layout(location = 0) out vec4 color;

// Values that stay constant for the whole mesh
layout(location = 1) uniform sampler2D DiffuseTex;

void main()
{
  color = texture(DiffuseTex, UV);
}

// vim: syntax=glsl
