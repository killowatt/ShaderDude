// Shader modified for SDE and is used for demonstration purposes only.
// https://www.shadertoy.com/view/ldycRK

#version 150 core

in vec2 fTexCoords;

out vec4 outColor;

uniform float time;
uniform vec2 resolution;
uniform vec4 variables;
uniform vec4 triggers;

uniform sampler2D diffuse;
uniform sampler2D bufferOne;

void main()
{
    vec2 uv = gl_FragCoord.xy/resolution.xy;
    vec2 p = vec2(uv.x, uv.y * (resolution.y/resolution.x));
    
    vec3 x = texture(bufferOne, uv).rgb;
    //x = 0.5*x + vec3(0.5);
    //x = pow(x, vec3(2.0))*4.0;
    //x = x*2.;
    outColor = vec4(x, 1.0);
}