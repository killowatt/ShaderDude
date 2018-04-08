#pragma once

#include "Graphics/Shader.h"

class GLFWwindow;
class StandardShader : public Shader
{
	int timeLocation;
	int resolutionLocation;

	int variablesLocation;

public:
	GLFWwindow* WindowReference;

	float a, b, c, d;

	void Initialize();
	void Update();

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};