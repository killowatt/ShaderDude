#pragma once

#include "Graphics/Shader.h"

class GLFWwindow;
class StandardShader : public Shader
{
	int timeLocation;
	int resolutionLocation;

public:
	GLFWwindow* WindowReference;

	void Initialize();
	void Update();

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};