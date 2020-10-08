#version 300 es

precision mediump float;

// Interpolated values from the vertex shader
in vec2 UV;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh
uniform sampler2D DiffuseTex;

void main()
{
  color = texture(DiffuseTex, UV);
}

// vim: syntax=glsl
