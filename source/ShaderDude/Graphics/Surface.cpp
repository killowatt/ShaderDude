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

uint32 Surface::GetVertexArray() const
{
	return vertexArray;
}
uint32 Surface::GetVertexBuffer() const
{
	return vertexBuffer;
}

void Surface::Bind()
{
	glBindVertexArray(vertexArray);
}

Surface::Surface()
{
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}