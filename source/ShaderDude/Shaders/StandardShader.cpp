#include "StandardShader.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

void StandardShader::Initialize()
{
	timeLocation = glGetUniformLocation(shaderProgram, "time");

	vec3 = glGetUniformLocation(shaderProgram, "col");
}
void StandardShader::Update()
{
	glUniform1f(timeLocation, glfwGetTime());
	glUniform3f(vec3, r, g, b);
}

StandardShader::StandardShader(const char* vertexFileName, const char* fragmentFileName) :
	Shader(ReadFile(vertexFileName), ReadFile(fragmentFileName))
{

}