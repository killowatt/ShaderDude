#pragma once
#include "ShaderDude.h"

class Surface
{
	uint32 vertexArray;
	uint32 vertexBuffer;

public:
	uint32 GetVertexArray() const;
	uint32 GetVertexBuffer() const;

	void Bind();

	Surface();
};