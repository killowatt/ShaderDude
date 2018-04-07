#include "StandardShader.h"

void StandardShader::Initialize()
{
}
void StandardShader::Update()
{
}

StandardShader::StandardShader(const char* vertexFileName, const char* fragmentFileName) :
	Shader(ReadFile(vertexFileName), ReadFile(fragmentFileName))
{

}