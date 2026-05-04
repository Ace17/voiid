#version 310 es

precision mediump float;

// Uniforms

layout(std140, binding=0) uniform MyUniformBlock
{
  int PassType;
};

layout(binding = 0) uniform sampler2D InputTex;

// Interpolated values from the vertex shader
layout(location = 0) in vec2 UV;

// Ouput data
layout(location = 0) out vec4 color;

const float kernel[10] = float[](
  0.1585, 0.1465, 0.1156, 0.0779, 0.0448, 0.0220, 0.0092, 0.0033, 0.0010, 0.0003
);

void main()
{
    vec3 col = vec3(0);

    if(PassType == 2)
    {
      vec3 srcColor = texture(InputTex, UV.xy).rgb;
      if(length(srcColor) < 3.5)
        col = vec3(0);
      else
        col = srcColor;
    }
    else
    {
      vec2 dir;
      if(PassType == 0) // horizontal
      {
        float dx = 1.0 / float(textureSize(InputTex, 0).x);
        dir = vec2(dx, 0);
      }
      else // vertical
      {
        float dy = 1.0 / float(textureSize(InputTex, 0).y);
        dir = vec2(0, dy);
      }

      col += kernel[0] * texture(InputTex, UV.xy).rgb;
      for(int i=1;i < 10;++i)
      {
        vec2 d = float(i) * dir;
        col += kernel[i] * texture(InputTex, UV.xy - d).rgb;
        col += kernel[i] * texture(InputTex, UV.xy + d).rgb;
      }
    }

    color = vec4(col, 1.0);
}

// vim: syntax=glsl
