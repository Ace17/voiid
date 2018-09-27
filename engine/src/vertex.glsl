#version 300 es

// Input vertex data, different for all executions of this shader
in vec4 a_position;
in vec2 a_texCoord;
in vec3 a_normal;

// Output data; will be interpolated for each fragment
out vec2 v_texCoord;
out vec3 vNormal;
out float fogFactor;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;

void main()
{
  gl_Position = MVP * a_position;
  v_texCoord = a_texCoord;
  vNormal = normalize(a_normal);
  fogFactor = clamp(1.0/exp(length(gl_Position) * 0.01), 0.0, 1.0);
}
// vim: syntax=glsl
