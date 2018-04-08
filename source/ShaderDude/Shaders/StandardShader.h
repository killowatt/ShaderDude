#pragma once

#include "Graphics/Shader.h"

class GLFWwindow;
class StandardShader : public Shader
{
	int timeLocation;
	int resolutionLocation;

	int variablesLocation;
	int triggersLocation;

public:
	GLFWwindow* WindowReference;

	float Variables[4];
	float Triggers[4];

	void Initialize();
	void Update();

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};