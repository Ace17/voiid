#version 310 es

precision mediump float;

// Uniforms
layout(std140, binding=0) uniform MyUniformBlock
{
  mat4 MVP;
};

precision mediump float;

// Input Vertex Attributes
layout(location = 0) in vec3 vPos;

// Ouput data
layout(location = 0) out vec4 color;

float hash( float n ) { return fract(sin(n)*123.456789); }

vec2 rotate( in vec2 uv, float a)
{
    float c = cos( a );
    float s = sin( a );
    return vec2( c * uv.x - s * uv.y, s * uv.x + c * uv.y );
}

float noise(in vec3 p)
{
    vec3 fl = floor(p);
    vec3 fr = fract(p);
    fr = fr * fr * ( 3.0 - 2.0 * fr );

    float n = fl.x + fl.y * 157.0 + 113.0 * fl.z;
    return mix( mix( mix( hash( n +   0.0), hash( n +   1.0 ), fr.x ),
                     mix( hash( n + 157.0), hash( n + 158.0 ), fr.x ), fr.y ),
                mix( mix( hash( n + 113.0), hash( n + 114.0 ), fr.x ),
                     mix( hash( n + 270.0), hash( n + 271.0 ), fr.x ), fr.y ), fr.z );
}

float fbm(in vec2 p)
{
    float f;
    f  = 0.5000 * noise( vec3( p, 0.0 ) ); p *= 3.1;
    f += 0.2500 * noise( vec3( p, 0.0 ) ); p *= 3.2;
    f += 0.1250 * noise( vec3( p, 0.0 ) ); p *= 3.3;
    f += 0.0625 * noise( vec3( p, 0.0 ) );
    return f;
}

float stars( in vec3 dir )
{
    vec3 n  = abs( dir );
    vec2 uv = ( n.x > n.y && n.x > n.z ) ? dir.yz / dir.x:
              ( n.y > n.x && n.y > n.z ) ? dir.zx / dir.y:
                                           dir.xy / dir.z;

    vec2 u = cos( 100. * uv ) * fbm( 10. * uv);
    return smoothstep( 0.5, 0.55, u.x * u.y );
}

void main()
{
  color.rgb = stars(normalize(vPos)) * vec3(1.0);
  //color.rgb = noise(vPos*100.0) * vec3(1.0);
  color.a = 1.0;
}

// vim: syntax=glsl
