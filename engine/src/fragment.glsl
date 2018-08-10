#version 100

precision mediump float;

// Interpolated values from the vertex shader
varying vec2 v_texCoord;
varying vec3 vNormal;
varying float fogFactor;

// Values that stay constant for the whole mesh
uniform vec4 v_color;
uniform sampler2D s_baseMap;
uniform vec3 ambientLight;

void main()
{
  vec3 lightDirEyeSpace = normalize(vec3(0.3,0.4,0.1));
  vec3 diffuseLight = vec3(max(0.0, dot(lightDirEyeSpace, vNormal)));
  vec3 light = diffuseLight + ambientLight;
  light = light * fogFactor;
  vec4 color = texture2D(s_baseMap, v_texCoord) + v_color;
  gl_FragColor = vec4(color.xyz * light, color.a);
}

// vim: syntax=glsl
