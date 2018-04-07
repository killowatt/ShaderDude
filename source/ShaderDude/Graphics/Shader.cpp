#include "Shader.h"
#include "GL/glew.h"

uint32 Shader::GetProgram() const
{
	return shaderProgram;
}
uint32 Shader::GetShader(ShaderType type) const
{
	if (type == ShaderType::Vertex)
		return vertexShader;
	else if (type == ShaderType::Fragment)
		return fragmentShader;
	return 0;
}


bool Shader::GetCompileStatus(ShaderType type) const
{
	int32 result = 0;
	if (type == ShaderType::Vertex)
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	else if (type == ShaderType::Fragment)
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	else
		return false;
	return result != 0;
}
std::string Shader::GetCompileLog(ShaderType type) const
{
	int logLength = 0;

	if (type == ShaderType::Vertex)
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);
	else if (type == ShaderType::Fragment)
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
	else
		return std::string("The shader type defined was invalid.");

	if (logLength > 0)
	{
		char* logBuffer = new char[logLength];
		if (type == ShaderType::Vertex)
			glGetShaderInfoLog(vertexShader, logLength, nullptr, logBuffer);
		else if (type == ShaderType::Fragment)
			glGetShaderInfoLog(fragmentShader, logLength, nullptr, logBuffer);

		std::string log(logBuffer);
		delete logBuffer;
		return log;
	}
	return std::string("The shader reported no errors or warnings.");
}

void Shader::Enable() const
{
	glUseProgram(shaderProgram);
}

Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource)
	: Shader(vertexSource.c_str(), fragmentSource.c_str())
{
}
Shader::Shader(const char* vertexSource, const char* fragmentSource)
{
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, nullptr);
	glCompileShader(vertexShader);

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glLinkProgram(shaderProgram);
}
Shader::~Shader()
{
	glDetachShader(shaderProgram, vertexShader);
	glDetachShader(shaderProgram, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(shaderProgram);
}