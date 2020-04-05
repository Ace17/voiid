#version 300 es

precision mediump float;

// Interpolated values from the vertex shader
in vec2 UV;
in vec2 UV_lightmap;
in vec3 vPos;
in vec3 vNormal;
in float fogFactor;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh
uniform vec4 fragOffset;
uniform sampler2D DiffuseTex;
uniform sampler2D LightmapTex;
uniform vec3 ambientLight;

void main()
{
  vec3 lightPos = vec3(2, 2, 2);
  vec3 lightDir = lightPos - vPos.xyz;
  float lightDist = length(lightDir);
  float attenuation = 1.0/lightDist;
  vec3 receivedLight = vec3(max(0.0, dot(normalize(lightDir), vNormal))) * attenuation;
  vec3 light = receivedLight + ambientLight * 0.2;
  light *= fogFactor;
  vec4 rawColor = texture2D(DiffuseTex, UV) + fragOffset;
  vec4 lightmapColor = texture2D(LightmapTex, UV_lightmap);
  color = vec4(rawColor.rgb * (light + lightmapColor.rgb*0.01), rawColor.a);
}

// vim: syntax=glsl
