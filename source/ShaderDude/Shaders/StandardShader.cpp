#include "StandardShader.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

void StandardShader::Initialize()
{
	timeLocation = glGetUniformLocation(shaderProgram, "time");
}
void StandardShader::Update()
{
	glUniform1f(timeLocation, glfwGetTime());
}

StandardShader::StandardShader(const char* vertexFileName, const char* fragmentFileName) :
	Shader(ReadFile(vertexFileName), ReadFile(fragmentFileName))
{

}