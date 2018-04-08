#include "StandardShader.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

void StandardShader::Initialize()
{
	timeLocation = glGetUniformLocation(shaderProgram, "time");
	resolutionLocation = glGetUniformLocation(shaderProgram, "resolution");
}
void StandardShader::Update()
{
	glUniform1f(timeLocation, glfwGetTime());
	int width, height;
	if (WindowReference)
		glfwGetWindowSize(WindowReference, &width, &height);
	glUniform2f(resolutionLocation, width, height);
}

StandardShader::StandardShader(const char* vertexFileName, const char* fragmentFileName) :
	Shader(ReadFile(vertexFileName), ReadFile(fragmentFileName))
{

}