#pragma once

#include "Graphics/Shader.h"

class StandardShader : public Shader
{
public:
	void Initialize();
	void Update();

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};