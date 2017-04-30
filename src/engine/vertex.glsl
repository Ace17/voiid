#version 100

attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec3 a_normal;

varying vec2 v_texCoord;
varying vec3 vNormal;

uniform mat4 MVP;

void main()
{
  gl_Position = MVP * a_position;
  v_texCoord = a_texCoord;
  vNormal = normalize(a_normal);
}
// vim: syntax=glsl
