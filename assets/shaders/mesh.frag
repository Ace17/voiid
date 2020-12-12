#version 310 es

precision mediump float;

// Uniforms
layout(location = 2) uniform vec3 CameraPos;
layout(location = 3) uniform vec4 fragOffset;
layout(location = 4) uniform sampler2D DiffuseTex;
layout(location = 5) uniform sampler2D LightmapTex;
layout(location = 6) uniform vec3 ambientLight;
layout(location = 7) uniform int LightCount;
layout(location = 8) uniform vec3 LightPos[32];
layout(location = 40) uniform vec3 LightColor[32];

// Interpolated values from the vertex shader
layout(location = 0) in vec2 UV;
layout(location = 1) in vec2 UV_lightmap;
layout(location = 2) in vec3 vPos;
layout(location = 3) in vec3 vNormal;

// Ouput data
layout(location = 0) out vec4 color;

void main()
{
  vec3 totalLight = vec3(0, 0, 0);

  // highlight
  totalLight += fragOffset.rgb;

  // ambient
  totalLight += vec3(1, 1, 1) * ambientLight * texture(DiffuseTex, UV).rgb;

  // lightmap
  totalLight += texture(LightmapTex, UV_lightmap).rgb * 0.01 * texture(DiffuseTex, UV).rgb;

  // dynamic lights
  for(int i=0;i < LightCount;++i)
  {
    vec3 lightDir = normalize(LightPos[i] - vPos);
    float lightDist = length(LightPos[i] - vPos);
    float attenuation = 10.0/(lightDist*lightDist*lightDist);
    float incidenceRatio = max(0.0, dot(lightDir, vNormal));

    // diffuse
    totalLight += LightColor[i] * incidenceRatio * attenuation * texture(DiffuseTex, UV).rgb;

    // specular
    const float material_shininess = 2048.0;
    const float material_specular = 0.001;
    vec3 viewDir = normalize(CameraPos - vPos);
    vec3 halfwayDir = normalize((viewDir + lightDir) * 0.5);
    float angle = max(dot(vNormal, halfwayDir), 0.0);
    float spec = pow(angle, material_shininess);
    totalLight += LightColor[i] * (spec * material_specular);
  }

  color.rgb = totalLight;
  color.a = texture(DiffuseTex, UV).a;

  if(false)
  {
    color.rgb = (vNormal+vec3(1))*0.5 + 0.0001 * color.rgb;
    color.a = 1.0;
  }
}

// vim: syntax=glsl
