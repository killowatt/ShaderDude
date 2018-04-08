#version 150 core
out vec4 outColor;

uniform float time;

void main()
{
    outColor = vec4(sin(time) * .96, 0.0, 0.96, 1.0);
}
