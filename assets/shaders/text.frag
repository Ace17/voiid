#version 310 es

precision mediump float;

// Uniforms
layout(std140, binding=0) uniform MyUniformBlock
{
  mat4 MVP;
};

layout(binding=1) uniform sampler2D DiffuseTex;

// Interpolated values from the vertex shader
layout(location = 0) in vec2 UV;

// Ouput data
layout(location = 0) out vec4 color;

void main()
{
  color = texture(DiffuseTex, UV);
}

// vim: syntax=glsl
