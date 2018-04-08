#include "StandardShader.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

void StandardShader::Initialize()
{
	timeLocation = glGetUniformLocation(shaderProgram, "time");
	resolutionLocation = glGetUniformLocation(shaderProgram, "resolution");

	variablesLocation = glGetUniformLocation(shaderProgram, "variables");
	triggersLocation = glGetUniformLocation(shaderProgram, "triggers");
}
void StandardShader::Update()
{
	glUniform1f(timeLocation, glfwGetTime());
	int width, height;
	if (WindowReference)
		glfwGetWindowSize(WindowReference, &width, &height);
	glUniform2f(resolutionLocation, width, height);

	glUniform4f(variablesLocation, Variables[0], Variables[1], Variables[2], Variables[3]);
	glUniform4f(triggersLocation, Triggers[0], Triggers[1], Triggers[2], Triggers[3]);
}

StandardShader::StandardShader(const char* vertexFileName, const char* fragmentFileName) :
	Shader(ReadFile(vertexFileName), ReadFile(fragmentFileName))
{

}