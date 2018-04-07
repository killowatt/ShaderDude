// vvv real stuff vvv
#include "ShaderDude.h"
#include <fstream>
#include <iostream>
#include <exception>

std::string ReadFile(const char* path)
{
	std::string file;
	std::ifstream fileStream(path, std::ios::in);
	if (fileStream.is_open())
	{
		std::string line = "";
		while (getline(fileStream, line))
			file += "\n" + line;
		fileStream.close();
		return file;
	}
	throw std::exception("we failed to load the file fam");
}




// Ex code below

// Link statically with GLEW

// Headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Shaders\StandardShader.h>

// Shader sources
const GLchar* vertexSource = R"glsl(
    #version 150 core
    in vec2 position;
    void main()
    {
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";
const GLchar* fragmentSource = R"glsl(
    #version 150 core
    out vec4 outColor;
    void main()
    {
        outColor = vec4(0, 1.0, 1.0, 1.0);
    }
)glsl";

int main()
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;


	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	// Initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Create Vertex Array Object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create a Vertex Buffer Object and copy the vertex data to it
	GLuint vbo;
	glGenBuffers(1, &vbo);

	GLfloat vertices[] =
	{
		-1, 1,
		1, 1,
		-1, -1,

		1, 1,
		1, -1,
		-1, -1
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	StandardShader shd("Shaders/StandardShader.vs", "Shaders/StandardShader.fs");
	shd.Enable();
	//// Create and compile the vertex shader
	//GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//glShaderSource(vertexShader, 1, &vertexSource, NULL);
	//glCompileShader(vertexShader);

	//// Create and compile the fragment shader
	//GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	//glCompileShader(fragmentShader);

	//// Link the vertex and fragment shader into a shader program
	//GLuint shaderProgram = glCreateProgram();
	//glAttachShader(shaderProgram, vertexShader);
	//glAttachShader(shaderProgram, fragmentShader);
	//glBindFragDataLocation(shaderProgram, 0, "outColor");
	//glLinkProgram(shaderProgram);
	//glUseProgram(shaderProgram);

	// Specify the layout of the vertex data
	GLint posAttrib = glGetAttribLocation(shd.GetProgram(), "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

	bool running = true;
	while (running)
	{
		// Clear the screen to black
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw a triangle from the 3 vertices
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//glDeleteProgram(shaderProgram);
	//glDeleteShader(fragmentShader);
	//glDeleteShader(vertexShader);

	glDeleteBuffers(1, &vbo);

	glDeleteVertexArrays(1, &vao);


	return 0;
}