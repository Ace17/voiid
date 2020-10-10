#version 310 es

precision mediump float;

// Interpolated values from the vertex shader
layout(location = 0) in vec2 UV;
layout(location = 1) in vec2 UV_lightmap;
layout(location = 2) in vec3 vPos;
layout(location = 3) in vec3 vNormal;

// Ouput data
layout(location = 0) out vec4 color;

// Values that stay constant for the whole mesh
layout(location = 2) uniform vec3 CameraPos;
layout(location = 3) uniform vec4 fragOffset;
layout(location = 4) uniform sampler2D DiffuseTex;
layout(location = 5) uniform sampler2D LightmapTex;
layout(location = 6) uniform vec3 ambientLight;
layout(location = 7) uniform vec3 LightPos;

void main()
{
  vec3 lightColor = vec3(1,1,1);
  vec3 lightDir = normalize(LightPos - vPos);
  float lightDist = length(LightPos - vPos);
  float attenuation = 10.0/(lightDist*lightDist*lightDist);

  // ambient
  vec3 ambient = texture(DiffuseTex, UV).rgb * ambientLight;

  // diffuse
  float diff = max(0.0, dot(lightDir, vNormal))*attenuation;
  diff += length(texture(LightmapTex, UV_lightmap)) * 0.01;
  vec3 diffuse = lightColor * (diff * (texture(DiffuseTex, UV).rgb + fragOffset.rgb));

  // specular
  const float material_shininess = 1024.0;
  const float material_specular = 0.4;
  vec3 viewDir = normalize(CameraPos - vPos);
  vec3 halfwayDir = normalize((viewDir + lightDir) * 0.5);
  float angle = max(dot(vNormal, halfwayDir), 0.0);
  float spec = pow(angle, material_shininess);
  vec3 specular = lightColor * (spec * material_specular);

  vec3 result = ambient + diffuse + specular;
  color = vec4(result, texture(DiffuseTex, UV).a);

  if(false)
  {
    color.rgb = (vNormal+vec3(1))*0.5 + 0.0001 * color.rgb;
    color.a = 1.0;
  }
}

// vim: syntax=glsl
