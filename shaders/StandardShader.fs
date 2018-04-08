#version 150 core

in vec2 fTexCoords;

out vec4 outColor;

uniform float time;

void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fTexCoords;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5 * cos(time +uv.xyx + vec3(0,2,4));

    // Output to screen
    outColor = vec4(col ,1.0);
}
