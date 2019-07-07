#version 300 es

precision mediump float;

// Interpolated values from the vertex shader
in vec2 UV;
in vec2 UV_lightmap;
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
  vec3 lightDirEyeSpace = normalize(vec3(0.3,0.4,0.1));
  vec3 receivedLight = vec3(max(0.0, dot(lightDirEyeSpace, vNormal)));
  vec3 light = receivedLight + ambientLight;
  light = light * fogFactor;
  vec4 rawColor = texture2D(DiffuseTex, UV) + fragOffset;
  vec4 lightmapColor = texture2D(LightmapTex, UV_lightmap);
  color = vec4(rawColor.rgb * (light*0.1 + lightmapColor.rgb), rawColor.a);
}

// vim: syntax=glsl
