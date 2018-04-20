#pragma once

#include "Graphics/Shader.h"

#include "glm/mat4x4.hpp"

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

	int VIEWMATRIXLOC;
	int PROJMATIRXLOC;

public:
	GLFWwindow* WindowReference;
	int frameBufferOneTex;

	glm::mat4 VIEW;
	glm::mat4 PROJ;

	double StartTime; // Sets start time

	float Variables[4];
	float Triggers[4];

	void Initialize();
	void Update();

	StandardShader(const char* vertexFileName, const char* fragmentFileName);
};