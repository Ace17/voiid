#version 310 es

precision highp float;

// Uniforms
layout(std140, binding=0) uniform MyUniformBlock
{
  mat4 MVP;
};

// Input Vertex Attributes
layout(location = 0) in vec3 vPos;

// Ouput data
layout(location = 0) out vec4 color;

// 3D Gradient noise from: https://www.shadertoy.com/view/Xsl3Dl
vec3 hash( vec3 p )
{
  p = vec3(
      dot(p,vec3(127.1,311.7, 74.7)),
      dot(p,vec3(269.5,183.3,246.1)),
      dot(p,vec3(113.5,271.9,124.6))
      );

  return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}
float noise( in vec3 p )
{
    vec3 i = floor( p );
    vec3 f = fract( p );

    vec3 u = f*f*(3.0-2.0*f);

    return mix( mix( mix( dot( hash( i + vec3(0,0,0) ), f - vec3(0,0,0) ),
                          dot( hash( i + vec3(1,0,0) ), f - vec3(1,0,0) ), u.x),
                     mix( dot( hash( i + vec3(0,1,0) ), f - vec3(0,1,0) ),
                          dot( hash( i + vec3(1,1,0) ), f - vec3(1,1,0) ), u.x), u.y),
                mix( mix( dot( hash( i + vec3(0,0,1) ), f - vec3(0,0,1) ),
                          dot( hash( i + vec3(1,0,1) ), f - vec3(1,0,1) ), u.x),
                     mix( dot( hash( i + vec3(0,1,1) ), f - vec3(0,1,1) ),
                          dot( hash( i + vec3(1,1,1) ), f - vec3(1,1,1) ), u.x), u.y), u.z );
}

void main()
{
  vec3 stars_direction = normalize(vPos);
  float stars_threshold = 12.0; // modifies the number of stars that are visible
  float stars_exposure = 200.0; // modifies the overall strength of the stars
  float stars = pow(clamp(noise(stars_direction * 200.0), 0.0, 1.0), stars_threshold) * stars_exposure;
  stars *= mix(0.4, 1.4, noise(stars_direction * 100.0));

  // Output to screen
  color = vec4(vec3(stars),1.0);
}
// vim: syntax=glsl
