#include "StandardShader.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "Graphics/lodepng.h"

void StandardShader::Initialize()
{
	timeLocation = glGetUniformLocation(shaderProgram, "time");
	resolutionLocation = glGetUniformLocation(shaderProgram, "resolution");

	variablesLocation = glGetUniformLocation(shaderProgram, "variables");
	triggersLocation = glGetUniformLocation(shaderProgram, "triggers");

	textureLocation = glGetUniformLocation(shaderProgram, "diffuse");

	glEnable(GL_TEXTURE_2D);
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_NEAREST = no smoothing
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	std::vector<unsigned char> image;
	unsigned int widthx, heightx;
	lodepng::decode(image, widthx, heightx, "texture.png");
	glTexImage2D(GL_TEXTURE_2D, 0, 4, widthx, heightx, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

	glActiveTexture(GL_TEXTURE0);

	glUniform1i(glGetUniformLocation(shaderProgram, "diffuse"), 0);
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