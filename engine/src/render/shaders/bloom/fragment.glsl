#version 300 es

precision mediump float;

// Interpolated values from the vertex shader
in vec2 UV;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh
uniform sampler2D InputTex;
uniform bool IsThreshold;

const float offset = 1.0 / 600.0;

void main()
{
    if(IsThreshold)
    {
      color = texture(InputTex, UV.xy);
      if(length(color.rgb) < 3.5)
        color = vec4(0, 0, 0, 1);
      return;
    }

    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right
    );

    float kernel[9] = float[](
        1.0, 2.0, 1.0,
        2.0, 4.0, 2.0,
        1.0, 2.0, 1.0
    );

    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(InputTex, UV.xy + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];

    col /= 16.0;

    color = vec4(col, 1.0);
}

// vim: syntax=glsl
