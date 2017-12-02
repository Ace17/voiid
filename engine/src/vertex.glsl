#version 100

attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec3 a_normal;

varying vec2 v_texCoord;
varying vec3 vNormal;
varying float fogFactor;

uniform mat4 MVP;

void main()
{
  gl_Position = MVP * a_position;
  v_texCoord = a_texCoord;
  vNormal = normalize(a_normal);
  fogFactor = clamp(1.0/exp(length(gl_Position) * 0.01), 0.0, 1.0);
}
// vim: syntax=glsl
