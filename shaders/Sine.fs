// SDE Sine Example

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
    vec2 coord = gl_FragCoord.xy / resolution.xy * 2. - 1.;

    float wave = pow(.0025 / abs((coord.y + sin((coord.x * 8 + (time * 2))) * .1)), 3);
    
    outColor = vec4(0, 0, 0, 1);
    outColor.g = wave;
}