#version 150 core
out vec4 outColor;

uniform float time;

//uniform vec3 col;

void main()
{
    outColor = vec4(sin(time), 12.0/255.0, 151.0/255.0, 1.0);
    //outColor = vec4(col, 1.0);
}
