#pragma once
#include "ShaderDude.h"

class Surface
{
	uint32 vertexArray;
	uint32 vertexBuffer;
	uint32 textureCoordinatesBuffer;

public:
	uint32 GetVertexArray() const;
	uint32 GetVertexBuffer() const;
	uint32 GetTextureCoordinatesBuffer() const;

	void Bind();

	Surface();
};