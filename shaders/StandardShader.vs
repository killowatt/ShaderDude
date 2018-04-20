// Default Vertex Shader for SDE

#version 150 core

in vec2 position;
in vec2 texCoords;

out vec2 fTexCoords;

uniform mat4 view;
uniform mat4 proj;

void main()
{
    fTexCoords = texCoords;
    gl_Position = proj * view * vec4(position, 0.0, 1.0);
}
