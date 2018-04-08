#include "Surface.h"
#include "GL/glew.h"

const GLfloat vertices[] =
{
	-1, 1,
	1, 1,
	-1, -1,
	1, 1,
	1, -1,
	-1, -1
};
const GLfloat textureCoordinates[] =
{
	0, 0,
	1, 0,
	0, 1,
	1, 0,
	1, 1,
	0, 1
};

uint32 Surface::GetVertexArray() const
{
	return vertexArray;
}
uint32 Surface::GetVertexBuffer() const
{
	return vertexBuffer;
}
uint32 Surface::GetTextureCoordinatesBuffer() const
{
	return textureCoordinatesBuffer;
}

void Surface::Bind()
{
	glBindVertexArray(vertexArray);
}

Surface::Surface()
{
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	// Vertices
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(0);

	// Tex Coords
	glGenBuffers(1, &textureCoordinatesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureCoordinatesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);

	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}