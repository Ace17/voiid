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
  vec3 lightColor = vec3(1,1,1);
  vec3 lightPos = vec3(2, 2, 2);
  vec3 lightDir = lightPos - vPos;
  float lightDist = length(lightDir);
  float attenuation = 10.0/(lightDist*lightDist);
  vec3 receivedLight = vec3(max(0.0, dot(normalize(lightDir), vNormal))) * attenuation;

  // ambient
  vec3 ambient = lightColor * ambientLight;

  // diffuse
  float diff = max(0.0, dot(normalize(lightDir), vNormal))*attenuation;
  diff += length(texture2D(LightmapTex, UV_lightmap)) * 0.01;
  diff *= fogFactor;
  vec3 diffuse = lightColor * (diff * (texture2D(DiffuseTex, UV).rgb + fragOffset.rgb));

  // specular
  // vec3 viewDir = normalize(viewPos - vPos);
  // vec3 reflectDir = reflect(-normalize(lightDir), vNormal);
  // float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  // vec3 specular = lightColor * (spec * material.specular);
  vec3 specular = vec3(0);

  vec3 result = ambient + diffuse + specular;
  color = vec4(result, texture2D(DiffuseTex, UV).a);

  if(false)
  {
    color.rgb = (vNormal+vec3(1))*0.5 + 0.0001 * color.rgb;
    color.a = 1.0;
  }
}

// vim: syntax=glsl
