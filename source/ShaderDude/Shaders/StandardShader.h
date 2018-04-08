#pragma once

#include "Graphics/Shader.h"

class StandardShader : public Shader
{
public:
	int timeLocation;

	void Initialize();
	void Update();

	int vec3;
	float r;
	float g;
	float b;

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};