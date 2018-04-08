#pragma once

#include "Graphics/Shader.h"

class StandardShader : public Shader
{
public:
	int timeLocation;

	void Initialize();
	void Update();

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};