#pragma once

#include "ShaderDude.h"
#include <string>

enum class ShaderType
{
	Vertex,
	Fragment
};
class Shader
{
protected:
	uint32 shaderProgram;
	uint32 vertexShader;
	uint32 fragmentShader;

public:
	uint32 GetProgram() const;
	uint32 GetShader(ShaderType type) const;

	std::string VertexFileName;
	std::string FragmentFileName;

	bool GetCompileStatus(ShaderType type) const;
	std::string GetCompileLog(ShaderType type) const;

	void Enable() const;
	void Recompile();

	virtual void Initialize() = 0; // TODO: should these be () const = 0;? probably?
	virtual void Update() = 0;

protected:
	Shader() = delete;
	Shader(const std::string& vertexSource, const std::string& fragmentSource);
	Shader(const char* vertexSource, const char* fragmentSource);

public:
	~Shader();
};