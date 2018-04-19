#pragma once

#include "Graphics/Shader.h"

class GLFWwindow;
class StandardShader : public Shader
{
	int timeLocation;
	int resolutionLocation;

	int variablesLocation;
	int triggersLocation;

	int textureLocation;
	uint32 tex;

	int frameBufferOneLocation;

public:
	GLFWwindow* WindowReference;
	int frameBufferOneTex;

	double StartTime; // Sets start time

	float Variables[4];
	float Triggers[4];

	void Initialize();
	void Update();

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};