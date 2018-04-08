#include "StandardShader.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

void StandardShader::Initialize()
{
	timeLocation = glGetUniformLocation(shaderProgram, "time");
	resolutionLocation = glGetUniformLocation(shaderProgram, "resolution");

	variablesLocation = glGetUniformLocation(shaderProgram, "variables");
}
void StandardShader::Update()
{
	glUniform1f(timeLocation, glfwGetTime());
	int width, height;
	if (WindowReference)
		glfwGetWindowSize(WindowReference, &width, &height);
	glUniform2f(resolutionLocation, width, height);

	glUniform4f(variablesLocation, a, b, c, d);
}

StandardShader::StandardShader(const char* vertexFileName, const char* fragmentFileName) :
	Shader(ReadFile(vertexFileName), ReadFile(fragmentFileName))
{

}