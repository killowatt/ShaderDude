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
	//throw std::exception("we failed to load the file fam");
	std::cout << "Couldn't find " << path << "\n";
	return std::string();
}




// Ex code below

// Link statically with GLEW

// Headers
#define GLFW_EXPOSE_NATIVE_WIN32

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Shaders\StandardShader.h>
#include <Graphics\Surface.h>

#include "imgui.h"
#include "GLFW\glfw3native.h"

#include "TextEditor.h"

//Oculus
#include <stdio.h>
#include <stdlib.h>

#include "Oculus/OVR_Avatar.h"

#include "Oculus/OVR_Platform.h"
#include "Oculus/OVR_PlatformVersion.h"
#include "Oculus/OVR_CAPI.h"
#include "Oculus/OVR_CAPI_GL.h"
#include "Oculus/OVR_Platform.h"
#include "Oculus\SDL.h"
#include "Oculus\SDL_opengl.h"

#include <Oculus/OVR_CAPI.h>
#include <Oculus/OVR_CAPI_GL.h>
#include <Oculus/OVR_Platform.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>
#include <chrono>

#define MIRROR_SAMPLE_APP_ID "958062084316416"
#define MIRROR_WINDOW_WIDTH 800
#define MIRROR_WINDOW_HEIGHT 600

// Disable MIRROR_ALLOW_OVR to force 2D rendering
#define MIRROR_ALLOW_OVR true

/************************************************************************************
* Static state
************************************************************************************/

static GLuint _skinnedMeshProgram;
static GLuint _combinedMeshProgram;
static GLuint _skinnedMeshPBSProgram;
static GLuint _debugLineProgram;
static GLuint _debugVertexArray;
static GLuint _debugVertexBuffer;
static ovrAvatar* _avatar;
static bool _combineMeshes = true;
static ovrAvatarAssetID _avatarCombinedMeshAlpha = 0;
static ovrAvatarVector4f _avatarCombinedMeshAlphaOffset;
static size_t _loadingAssets;
static bool _waitingOnCombinedMesh = false;

static float _elapsedSeconds;
static std::map<ovrAvatarAssetID, void*> _assetMap;

//




static glm::vec3 _glmFromOvrVector(const ovrVector3f& ovrVector)
{
	return glm::vec3(ovrVector.x, ovrVector.y, ovrVector.z);
}

static glm::quat _glmFromOvrQuat(const ovrQuatf& ovrQuat)
{
	return glm::quat(ovrQuat.w, ovrQuat.x, ovrQuat.y, ovrQuat.z);
}

static void _glmFromOvrAvatarTransform(const ovrAvatarTransform& transform, glm::mat4* target) {
	glm::vec3 position(transform.position.x, transform.position.y, transform.position.z);
	glm::quat orientation(transform.orientation.w, transform.orientation.x, transform.orientation.y, transform.orientation.z);
	glm::vec3 scale(transform.scale.x, transform.scale.y, transform.scale.z);
	*target = glm::translate(position) * glm::mat4_cast(orientation) * glm::scale(scale);
}

static void _ovrAvatarTransformFromGlm(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& scale, ovrAvatarTransform* target) {
	target->position.x = position.x;
	target->position.y = position.y;
	target->position.z = position.z;
	target->orientation.x = orientation.x;
	target->orientation.y = orientation.y;
	target->orientation.z = orientation.z;
	target->orientation.w = orientation.w;
	target->scale.x = scale.x;
	target->scale.y = scale.y;
	target->scale.z = scale.z;
}

static void _ovrAvatarTransformFromGlm(const glm::mat4& matrix, ovrAvatarTransform* target) {
	glm::vec3 scale;
	glm::quat orientation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, orientation, translation, skew, perspective);
	_ovrAvatarTransformFromGlm(translation, orientation, scale, target);
}

static void _ovrAvatarHandInputStateFromOvr(const ovrAvatarTransform& transform, const ovrInputState& inputState, ovrHandType hand, ovrAvatarHandInputState* state)
{
	state->transform = transform;
	state->buttonMask = 0;
	state->touchMask = 0;
	state->joystickX = inputState.Thumbstick[hand].x;
	state->joystickY = inputState.Thumbstick[hand].y;
	state->indexTrigger = inputState.IndexTrigger[hand];
	state->handTrigger = inputState.HandTrigger[hand];
	state->isActive = false;
	if (hand == ovrHand_Left)
	{
		if (inputState.Buttons & ovrButton_X) state->buttonMask |= ovrAvatarButton_One;
		if (inputState.Buttons & ovrButton_Y) state->buttonMask |= ovrAvatarButton_Two;
		if (inputState.Buttons & ovrButton_Enter) state->buttonMask |= ovrAvatarButton_Three;
		if (inputState.Buttons & ovrButton_LThumb) state->buttonMask |= ovrAvatarButton_Joystick;
		if (inputState.Touches & ovrTouch_X) state->touchMask |= ovrAvatarTouch_One;
		if (inputState.Touches & ovrTouch_Y) state->touchMask |= ovrAvatarTouch_Two;
		if (inputState.Touches & ovrTouch_LThumb) state->touchMask |= ovrAvatarTouch_Joystick;
		if (inputState.Touches & ovrTouch_LThumbRest) state->touchMask |= ovrAvatarTouch_ThumbRest;
		if (inputState.Touches & ovrTouch_LIndexTrigger) state->touchMask |= ovrAvatarTouch_Index;
		if (inputState.Touches & ovrTouch_LIndexPointing) state->touchMask |= ovrAvatarTouch_Pointing;
		if (inputState.Touches & ovrTouch_LThumbUp) state->touchMask |= ovrAvatarTouch_ThumbUp;
		state->isActive = (inputState.ControllerType & ovrControllerType_LTouch) != 0;
	}
	else if (hand == ovrHand_Right)
	{
		if (inputState.Buttons & ovrButton_A) state->buttonMask |= ovrAvatarButton_One;
		if (inputState.Buttons & ovrButton_B) state->buttonMask |= ovrAvatarButton_Two;
		if (inputState.Buttons & ovrButton_Home) state->buttonMask |= ovrAvatarButton_Three;
		if (inputState.Buttons & ovrButton_RThumb) state->buttonMask |= ovrAvatarButton_Joystick;
		if (inputState.Touches & ovrTouch_A) state->touchMask |= ovrAvatarTouch_One;
		if (inputState.Touches & ovrTouch_B) state->touchMask |= ovrAvatarTouch_Two;
		if (inputState.Touches & ovrTouch_RThumb) state->touchMask |= ovrAvatarTouch_Joystick;
		if (inputState.Touches & ovrTouch_RThumbRest) state->touchMask |= ovrAvatarTouch_ThumbRest;
		if (inputState.Touches & ovrTouch_RIndexTrigger) state->touchMask |= ovrAvatarTouch_Index;
		if (inputState.Touches & ovrTouch_RIndexPointing) state->touchMask |= ovrAvatarTouch_Pointing;
		if (inputState.Touches & ovrTouch_RThumbUp) state->touchMask |= ovrAvatarTouch_ThumbUp;
		state->isActive = (inputState.ControllerType & ovrControllerType_RTouch) != 0;
	}
}

static void _computeWorldPose(const ovrAvatarSkinnedMeshPose& localPose, glm::mat4* worldPose)
{
	for (uint32_t i = 0; i < localPose.jointCount; ++i)
	{
		glm::mat4 local;
		_glmFromOvrAvatarTransform(localPose.jointTransform[i], &local);

		int parentIndex = localPose.jointParents[i];
		if (parentIndex < 0)
		{
			worldPose[i] = local;
		}
		else
		{
			worldPose[i] = worldPose[parentIndex] * local;
		}
	}
}

static glm::mat4 _computeReflectionMatrix(const glm::vec4& plane)
{
	return glm::mat4(
		1.0f - 2.0f * plane.x * plane.x,
		-2.0f * plane.x * plane.y,
		-2.0f * plane.x * plane.z,
		-2.0f * plane.w * plane.x,

		-2.0f * plane.y * plane.x,
		1.0f - 2.0f * plane.y * plane.y,
		-2.0f * plane.y * plane.z,
		-2.0f * plane.w * plane.y,

		-2.0f * plane.z * plane.x,
		-2.0f * plane.z * plane.y,
		1.0f - 2.0f * plane.z * plane.z,
		-2.0f * plane.w * plane.z,

		0.0f,
		0.0f,
		0.0f,
		1.0f
	);
}









//

// Shader sources

// IMGUI SHADERS
const GLchar* vertex_shader =
"uniform mat4 ProjMtx;\n"
"in vec2 Position;\n"
"in vec2 UV;\n"
"in vec4 Color;\n"
"out vec2 Frag_UV;\n"
"out vec4 Frag_Color;\n"
"void main()\n"
"{\n"
"	Frag_UV = UV;\n"
"	Frag_Color = Color;\n"
"	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
"}\n";

const GLchar* fragment_shader =
"uniform sampler2D Texture;\n"
"in vec2 Frag_UV;\n"
"in vec4 Frag_Color;\n"
"out vec4 Out_Color;\n"
"void main()\n"
"{\n"
"	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
"}\n";

// MORE IMGUI stuff

static char         g_GlslVersion[32] = "#version 150";
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_ElementsHandle = 0;

static float gtime;

static GLFWcursor*  g_MouseCursors[ImGuiMouseCursor_COUNT] = { 0 };
static bool         g_MouseJustPressed[3] = { false, false, false };

void ImGui_ImplGlfwGL3_RenderDrawData(ImDrawData* draw_data)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);


	// Backup GL state
	GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
	glActiveTexture(GL_TEXTURE0);
	GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
	GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
	GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
	GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
	GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
	GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
	GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
	GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
	GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
	GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Setup viewport, orthographic projection matrix
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
	const float ortho_projection[4][4] =
	{
		{ 2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
	{ 0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
	{ 0.0f,                  0.0f,                  -1.0f, 0.0f },
	{ -1.0f,                  1.0f,                   0.0f, 1.0f },
	};
	glUseProgram(g_ShaderHandle);
	glUniform1i(g_AttribLocationTex, 0);
	glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
	glBindSampler(0, 0); // Rely on combined texture/sampler state.

						 // Recreate the VAO every time 
						 // (This is to easily allow multiple GL contexts. VAO are not shared among GL contexts, and we don't track creation/deletion of windows so we don't have an obvious key to use to cache them.)
	GLuint vao_handle = 0;
	glGenVertexArrays(1, &vao_handle);
	glBindVertexArray(vao_handle);
	glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
	glEnableVertexAttribArray(g_AttribLocationPosition);
	glEnableVertexAttribArray(g_AttribLocationUV);
	glEnableVertexAttribArray(g_AttribLocationColor);
	glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

	// Draw
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawIdx* idx_buffer_offset = 0;

		glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
			}
			idx_buffer_offset += pcmd->ElemCount;
		}
	}
	glDeleteVertexArrays(1, &vao_handle);

	// Restore modified GL state
	glUseProgram(last_program);
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindSampler(0, last_sampler);
	glActiveTexture(last_active_texture);
	glBindVertexArray(last_vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
	if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

bool ImGui_ImplGlfwGL3_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

															  // Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);

	return true;
}

bool ImGui_ImplGlfwGL3_CreateDeviceObjects()
{
	// Backup GL state
	GLint last_texture, last_array_buffer, last_vertex_array;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

	const GLchar* vertex_shader =
		"#version 150\n"
		"uniform mat4 ProjMtx;\n"
		"in vec2 Position;\n"
		"in vec2 UV;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	Frag_UV = UV;\n"
		"	Frag_Color = Color;\n"
		"	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

	const GLchar* fragment_shader =
		"#version 150\n"
		"uniform sampler2D Texture;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
		"}\n";

	//const GLchar* vertex_shader_with_version[2] = { g_GlslVersion, vertex_shader };
	//const GLchar* fragment_shader_with_version[2] = { g_GlslVersion, fragment_shader };

	g_ShaderHandle = glCreateProgram();
	g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
	g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(g_VertHandle, 1, &vertex_shader, NULL);
	glShaderSource(g_FragHandle, 1, &fragment_shader, NULL);
	glCompileShader(g_VertHandle);
	glCompileShader(g_FragHandle);
	glAttachShader(g_ShaderHandle, g_VertHandle);
	glAttachShader(g_ShaderHandle, g_FragHandle);

	glLinkProgram(g_ShaderHandle);

	g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
	g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
	g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
	g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
	g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

	glGenBuffers(1, &g_VboHandle);
	glGenBuffers(1, &g_ElementsHandle);

	ImGui_ImplGlfwGL3_CreateFontsTexture();

	// Restore modified GL state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindVertexArray(last_vertex_array);

	return true;
}

void ImGui_ImplGlfwGL3_NewFrame(GLFWwindow* window, float& g_Time)
{
	if (!g_FontTexture)
		ImGui_ImplGlfwGL3_CreateDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(window, &w, &h);
	glfwGetFramebufferSize(window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

	// Setup time step
	double current_time = glfwGetTime();
	io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
	g_Time = current_time;

	// Setup inputs
	// (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
	if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
	{
		// Set OS mouse position if requested (only used when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
		if (io.WantSetMousePos)
		{
			glfwSetCursorPos(window, (double)io.MousePos.x, (double)io.MousePos.y);
		}
		else
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos(window, &mouse_x, &mouse_y);
			io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
		}
	}
	else
	{
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	}

	for (int i = 0; i < 3; i++)
	{
		// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
		io.MouseDown[i] = g_MouseJustPressed[i] || glfwGetMouseButton(window, i) != 0;
		g_MouseJustPressed[i] = false;
	}

	// Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
	if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0 && glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
	{
		ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
		if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
		else
		{
			//glfwSetCursor(window, g_MouseCursors[cursor] ? g_MouseCursors[cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	// Gamepad navigation mapping [BETA]
	memset(io.NavInputs, 0, sizeof(io.NavInputs));
	if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)
	{
		// Update gamepad inputs
#define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
		int axes_count = 0, buttons_count = 0;
		const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
		const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
		MAP_BUTTON(ImGuiNavInput_Activate, 0);     // Cross / A
		MAP_BUTTON(ImGuiNavInput_Cancel, 1);     // Circle / B
		MAP_BUTTON(ImGuiNavInput_Menu, 2);     // Square / X
		MAP_BUTTON(ImGuiNavInput_Input, 3);     // Triangle / Y
		MAP_BUTTON(ImGuiNavInput_DpadLeft, 13);    // D-Pad Left
		MAP_BUTTON(ImGuiNavInput_DpadRight, 11);    // D-Pad Right
		MAP_BUTTON(ImGuiNavInput_DpadUp, 10);    // D-Pad Up
		MAP_BUTTON(ImGuiNavInput_DpadDown, 12);    // D-Pad Down
		MAP_BUTTON(ImGuiNavInput_FocusPrev, 4);     // L1 / LB
		MAP_BUTTON(ImGuiNavInput_FocusNext, 5);     // R1 / RB
		MAP_BUTTON(ImGuiNavInput_TweakSlow, 4);     // L1 / LB
		MAP_BUTTON(ImGuiNavInput_TweakFast, 5);     // R1 / RB
		MAP_ANALOG(ImGuiNavInput_LStickLeft, 0, -0.3f, -0.9f);
		MAP_ANALOG(ImGuiNavInput_LStickRight, 0, +0.3f, +0.9f);
		MAP_ANALOG(ImGuiNavInput_LStickUp, 1, +0.3f, +0.9f);
		MAP_ANALOG(ImGuiNavInput_LStickDown, 1, -0.3f, -0.9f);
#undef MAP_BUTTON
#undef MAP_ANALOG
		if (axes_count > 0 && buttons_count > 0)
			io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
		else
			io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
	}

	// Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
	ImGui::NewFrame();
}

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
	if (action == GLFW_PRESS && button >= 0 && button < 3)
		g_MouseJustPressed[button] = true;
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH += (float)xoffset;
	io.MouseWheel += (float)yoffset;
}

void ImGui_ImplGlfw_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS)
		io.KeysDown[key] = true;
	if (action == GLFW_RELEASE)
		io.KeysDown[key] = false;

	(void)mods; // Modifiers are not reliable across systems
	io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
	io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
	io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
	io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow*, unsigned int c)
{
	ImGuiIO& io = ImGui::GetIO();
	if (c > 0 && c < 0x10000)
		io.AddInputCharacter((unsigned short)c);
}

static void ImGui_ImplGlfw_InstallCallbacks(GLFWwindow* window)
{
	glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
	glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
	glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
	glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
}


bool    ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks, const char* glsl_version)
{
	//g_Window = window;

	// Store GL version string so we can refer to it later in case we recreate shaders.
	if (glsl_version == NULL)
		glsl_version = "#version 150";
	IM_ASSERT((int)strlen(glsl_version) + 2 < IM_ARRAYSIZE(g_GlslVersion));
	//strcpy(g_GlslVersion, glsl_version);
	//strcat(g_GlslVersion, "\n");
	glsl_version = "#version 150";


	// Setup back-end capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;   // We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;    // We can honor io.WantSetMousePos requests (optional, rarely used)
															// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	//io.SetClipboardTextFn = ImGui_ImplGlfwGL3_SetClipboardText;
	//io.GetClipboardTextFn = ImGui_ImplGlfwGL3_GetClipboardText;
	io.ClipboardUserData = window;
#ifdef _WIN32
	io.ImeWindowHandle = glfwGetWin32Window(window);
#endif

	// Load cursors
	// FIXME: GLFW doesn't expose suitable cursors for ResizeAll, ResizeNESW, ResizeNWSE. We revert to arrow cursor for those.
	g_MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

	if (install_callbacks)
		ImGui_ImplGlfw_InstallCallbacks(window);

	return true;
}


/************************************************************************************
* Wrappers for GL representations of avatar assets
************************************************************************************/

struct MeshData {
	GLuint vertexArray;
	GLuint vertexBuffer;
	GLuint elementBuffer;
	GLuint elementCount;
	glm::mat4 bindPose[OVR_AVATAR_MAXIMUM_JOINT_COUNT];
	glm::mat4 inverseBindPose[OVR_AVATAR_MAXIMUM_JOINT_COUNT];
};

struct TextureData {
	GLuint textureID;
};

static MeshData* _loadCombinedMesh(const ovrAvatarMeshAssetDataV2* data)
{
	_waitingOnCombinedMesh = false;

	MeshData* mesh = new MeshData();

	// Create the vertex array and buffer
	glGenVertexArrays(1, &mesh->vertexArray);
	glGenBuffers(1, &mesh->vertexBuffer);
	glGenBuffers(1, &mesh->elementBuffer);

	// Bind the vertex buffer and assign the vertex data	
	glBindVertexArray(mesh->vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, data->vertexCount * sizeof(ovrAvatarMeshVertexV2), data->vertexBuffer, GL_STATIC_DRAW);

	// Bind the index buffer and assign the index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, data->indexCount * sizeof(GLushort), data->indexBuffer, GL_STATIC_DRAW);
	mesh->elementCount = data->indexCount;

	// Fill in the array attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertexV2), &((ovrAvatarMeshVertexV2*)0)->x);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertexV2), &((ovrAvatarMeshVertexV2*)0)->nx);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertexV2), &((ovrAvatarMeshVertexV2*)0)->tx);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertexV2), &((ovrAvatarMeshVertexV2*)0)->u);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 4, GL_BYTE, GL_FALSE, sizeof(ovrAvatarMeshVertexV2), &((ovrAvatarMeshVertexV2*)0)->blendIndices);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertexV2), &((ovrAvatarMeshVertexV2*)0)->blendWeights);
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertexV2), &((ovrAvatarMeshVertexV2*)0)->r);
	glEnableVertexAttribArray(6);

	// Clean up
	glBindVertexArray(0);

	// Translate the bind pose
	_computeWorldPose(data->skinnedBindPose, mesh->bindPose);
	for (uint32_t i = 0; i < data->skinnedBindPose.jointCount; ++i)
	{
		mesh->inverseBindPose[i] = glm::inverse(mesh->bindPose[i]);
	}
	return mesh;
}

static MeshData* _loadMesh(const ovrAvatarMeshAssetData* data)
{
	MeshData* mesh = new MeshData();

	// Create the vertex array and buffer
	glGenVertexArrays(1, &mesh->vertexArray);
	glGenBuffers(1, &mesh->vertexBuffer);
	glGenBuffers(1, &mesh->elementBuffer);

	// Bind the vertex buffer and assign the vertex data	
	glBindVertexArray(mesh->vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, data->vertexCount * sizeof(ovrAvatarMeshVertex), data->vertexBuffer, GL_STATIC_DRAW);

	// Bind the index buffer and assign the index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, data->indexCount * sizeof(GLushort), data->indexBuffer, GL_STATIC_DRAW);
	mesh->elementCount = data->indexCount;

	// Fill in the array attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertex), &((ovrAvatarMeshVertex*)0)->x);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertex), &((ovrAvatarMeshVertex*)0)->nx);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertex), &((ovrAvatarMeshVertex*)0)->tx);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertex), &((ovrAvatarMeshVertex*)0)->u);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 4, GL_BYTE, GL_FALSE, sizeof(ovrAvatarMeshVertex), &((ovrAvatarMeshVertex*)0)->blendIndices);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ovrAvatarMeshVertex), &((ovrAvatarMeshVertex*)0)->blendWeights);
	glEnableVertexAttribArray(5);

	// Clean up
	glBindVertexArray(0);

	// Translate the bind pose
	_computeWorldPose(data->skinnedBindPose, mesh->bindPose);
	for (uint32_t i = 0; i < data->skinnedBindPose.jointCount; ++i)
	{
		mesh->inverseBindPose[i] = glm::inverse(mesh->bindPose[i]);
	}
	return mesh;
}

static TextureData* _loadTexture(const ovrAvatarTextureAssetData* data)
{
	// Create a texture
	TextureData* texture = new TextureData();
	glGenTextures(1, &texture->textureID);
	glBindTexture(GL_TEXTURE_2D, texture->textureID);

	// Load the image data
	switch (data->format)
	{
		// Handle uncompressed image data
	case ovrAvatarTextureFormat_RGB24:
		for (uint32_t level = 0, offset = 0, width = data->sizeX, height = data->sizeY; level < data->mipCount; ++level)
		{
			glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data->textureData + offset);
			offset += width * height * 3;
			width /= 2;
			height /= 2;
		}
		break;

		// Handle compressed image data
	case ovrAvatarTextureFormat_DXT1:
	case ovrAvatarTextureFormat_DXT5:
	{
		GLenum glFormat;
		int blockSize;
		if (data->format == ovrAvatarTextureFormat_DXT1)
		{
			blockSize = 8;
			glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		}
		else
		{
			blockSize = 16;
			glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}

		for (uint32_t level = 0, offset = 0, width = data->sizeX, height = data->sizeY; level < data->mipCount; ++level)
		{
			GLsizei levelSize = (width < 4 || height < 4) ? blockSize : blockSize * (width / 4) * (height / 4);
			glCompressedTexImage2D(GL_TEXTURE_2D, level, glFormat, width, height, 0, levelSize, data->textureData + offset);
			offset += levelSize;
			width /= 2;
			height /= 2;
		}
		break;
	}

	// Handle ASTC data
	case ovrAvatarTextureFormat_ASTC_RGB_6x6_MIPMAPS:
	{
		const unsigned char * level = (const unsigned char*)data->textureData;

		unsigned int w = data->sizeX;
		unsigned int h = data->sizeY;
		for (unsigned int i = 0; i < data->mipCount; i++)
		{
			int32_t blocksWide = (w + 5) / 6;
			int32_t blocksHigh = (h + 5) / 6;
			int32_t mipSize = 16 * blocksWide * blocksHigh;

			glCompressedTexImage2D(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_ASTC_6x6_KHR, w, h, 0, mipSize, level);

			level += mipSize;

			w >>= 1;
			h >>= 1;
			if (w < 1) { w = 1; }
			if (h < 1) { h = 1; }
		}
		break;
	}

	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	return texture;
}

/************************************************************************************
* Rendering functions
************************************************************************************/

static void _setTextureSampler(GLuint program, int textureUnit, const char uniformName[], ovrAvatarAssetID assetID)
{
	GLuint textureID = 0;
	if (assetID)
	{
		void* data = _assetMap[assetID];
		TextureData* textureData = (TextureData*)data;
		textureID = textureData->textureID;
	}
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glUniform1i(glGetUniformLocation(program, uniformName), textureUnit);
}

static void _setTextureSamplers(GLuint program, const char uniformName[], size_t count, const int textureUnits[], const ovrAvatarAssetID assetIDs[])
{
	for (int i = 0; i < count; ++i)
	{
		ovrAvatarAssetID assetID = assetIDs[i];

		GLuint textureID = 0;
		if (assetID)
		{
			void* data = _assetMap[assetID];
			if (data)
			{
				TextureData* textureData = (TextureData*)data;
				textureID = textureData->textureID;
			}
		}
		glActiveTexture(GL_TEXTURE0 + textureUnits[i]);
		glBindTexture(GL_TEXTURE_2D, textureID);
	}
	GLint uniformLocation = glGetUniformLocation(program, uniformName);
	glUniform1iv(uniformLocation, (GLsizei)count, textureUnits);
}

static void _setMeshState(
	GLuint program,
	const ovrAvatarTransform& localTransform,
	const MeshData* data,
	const ovrAvatarSkinnedMeshPose& skinnedPose,
	const glm::mat4& world,
	const glm::mat4& view,
	const glm::mat4 proj,
	const glm::vec3& viewPos
) {
	// Compute the final world and viewProjection matrices for this part
	glm::mat4 local;
	_glmFromOvrAvatarTransform(localTransform, &local);
	glm::mat4 worldMat = world * local;
	glm::mat4 viewProjMat = proj * view;

	// Compute the skinned pose
	glm::mat4* skinnedPoses = (glm::mat4*)alloca(sizeof(glm::mat4) * skinnedPose.jointCount);
	_computeWorldPose(skinnedPose, skinnedPoses);
	for (uint32_t i = 0; i < skinnedPose.jointCount; ++i)
	{
		skinnedPoses[i] = skinnedPoses[i] * data->inverseBindPose[i];
	}

	// Pass the world view position to the shader for view-dependent rendering
	glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(viewPos));

	// Assign the vertex uniforms
	glUniformMatrix4fv(glGetUniformLocation(program, "world"), 1, 0, glm::value_ptr(worldMat));
	glUniformMatrix4fv(glGetUniformLocation(program, "viewProj"), 1, 0, glm::value_ptr(viewProjMat));
	glUniformMatrix4fv(glGetUniformLocation(program, "meshPose"), (GLsizei)skinnedPose.jointCount, 0, glm::value_ptr(*skinnedPoses));
}

static void _setMaterialState(GLuint program, const ovrAvatarMaterialState* state, glm::mat4* projectorInv)
{
	// Assign the fragment uniforms
	glUniform1i(glGetUniformLocation(program, "useAlpha"), state->alphaMaskTextureID != 0);
	glUniform1i(glGetUniformLocation(program, "useNormalMap"), state->normalMapTextureID != 0);
	glUniform1i(glGetUniformLocation(program, "useRoughnessMap"), state->roughnessMapTextureID != 0);

	glUniform1f(glGetUniformLocation(program, "elapsedSeconds"), _elapsedSeconds);

	if (projectorInv)
	{
		glUniform1i(glGetUniformLocation(program, "useProjector"), 1);
		glUniformMatrix4fv(glGetUniformLocation(program, "projectorInv"), 1, 0, glm::value_ptr(*projectorInv));
	}
	else
	{
		glUniform1i(glGetUniformLocation(program, "useProjector"), 0);
	}

	int textureSlot = 1;
	glUniform4fv(glGetUniformLocation(program, "baseColor"), 1, &state->baseColor.x);
	glUniform1i(glGetUniformLocation(program, "baseMaskType"), state->baseMaskType);
	glUniform4fv(glGetUniformLocation(program, "baseMaskParameters"), 1, &state->baseMaskParameters.x);
	glUniform4fv(glGetUniformLocation(program, "baseMaskAxis"), 1, &state->baseMaskAxis.x);
	_setTextureSampler(program, textureSlot++, "alphaMask", state->alphaMaskTextureID);
	glUniform4fv(glGetUniformLocation(program, "alphaMaskScaleOffset"), 1, &state->alphaMaskScaleOffset.x);
	_setTextureSampler(program, textureSlot++, "clothingAlpha", _avatarCombinedMeshAlpha);
	glUniform4fv(glGetUniformLocation(program, "clothingAlphaScaleOffset"), 1, &_avatarCombinedMeshAlphaOffset.x);
	_setTextureSampler(program, textureSlot++, "normalMap", state->normalMapTextureID);
	glUniform4fv(glGetUniformLocation(program, "normalMapScaleOffset"), 1, &state->normalMapScaleOffset.x);
	_setTextureSampler(program, textureSlot++, "parallaxMap", state->parallaxMapTextureID);
	glUniform4fv(glGetUniformLocation(program, "parallaxMapScaleOffset"), 1, &state->parallaxMapScaleOffset.x);
	_setTextureSampler(program, textureSlot++, "roughnessMap", state->roughnessMapTextureID);
	glUniform4fv(glGetUniformLocation(program, "roughnessMapScaleOffset"), 1, &state->roughnessMapScaleOffset.x);

	struct LayerUniforms {
		int layerSamplerModes[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		int layerBlendModes[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		int layerMaskTypes[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		ovrAvatarVector4f layerColors[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		int layerSurfaces[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		ovrAvatarAssetID layerSurfaceIDs[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		ovrAvatarVector4f layerSurfaceScaleOffsets[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		ovrAvatarVector4f layerSampleParameters[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		ovrAvatarVector4f layerMaskParameters[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
		ovrAvatarVector4f layerMaskAxes[OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT];
	} layerUniforms;
	memset(&layerUniforms, 0, sizeof(layerUniforms));
	for (uint32_t i = 0; i < state->layerCount; ++i)
	{
		const ovrAvatarMaterialLayerState& layerState = state->layers[i];
		layerUniforms.layerSamplerModes[i] = layerState.sampleMode;
		layerUniforms.layerBlendModes[i] = layerState.blendMode;
		layerUniforms.layerMaskTypes[i] = layerState.maskType;
		layerUniforms.layerColors[i] = layerState.layerColor;
		layerUniforms.layerSurfaces[i] = textureSlot++;
		layerUniforms.layerSurfaceIDs[i] = layerState.sampleTexture;
		layerUniforms.layerSurfaceScaleOffsets[i] = layerState.sampleScaleOffset;
		layerUniforms.layerSampleParameters[i] = layerState.sampleParameters;
		layerUniforms.layerMaskParameters[i] = layerState.maskParameters;
		layerUniforms.layerMaskAxes[i] = layerState.maskAxis;
	}

	glUniform1i(glGetUniformLocation(program, "layerCount"), state->layerCount);
	glUniform1iv(glGetUniformLocation(program, "layerSamplerModes"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, layerUniforms.layerSamplerModes);
	glUniform1iv(glGetUniformLocation(program, "layerBlendModes"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, layerUniforms.layerBlendModes);
	glUniform1iv(glGetUniformLocation(program, "layerMaskTypes"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, layerUniforms.layerMaskTypes);
	glUniform4fv(glGetUniformLocation(program, "layerColors"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, (float*)layerUniforms.layerColors);
	_setTextureSamplers(program, "layerSurfaces", OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, layerUniforms.layerSurfaces, layerUniforms.layerSurfaceIDs);
	glUniform4fv(glGetUniformLocation(program, "layerSurfaceScaleOffsets"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, (float*)layerUniforms.layerSurfaceScaleOffsets);
	glUniform4fv(glGetUniformLocation(program, "layerSampleParameters"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, (float*)layerUniforms.layerSampleParameters);
	glUniform4fv(glGetUniformLocation(program, "layerMaskParameters"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, (float*)layerUniforms.layerMaskParameters);
	glUniform4fv(glGetUniformLocation(program, "layerMaskAxes"), OVR_AVATAR_MAX_MATERIAL_LAYER_COUNT, (float*)layerUniforms.layerMaskAxes);

}

static void _setPBSState(GLuint program, const ovrAvatarAssetID albedoTextureID, const ovrAvatarAssetID surfaceTextureID)
{
	int textureSlot = 0;
	_setTextureSampler(program, textureSlot++, "albedo", albedoTextureID);
	_setTextureSampler(program, textureSlot++, "surface", surfaceTextureID);
}

static void _renderDebugLine(const glm::mat4& worldViewProj, const glm::vec3& a, const glm::vec3& b, const glm::vec4& aColor, const glm::vec4& bColor)
{
	glUseProgram(_debugLineProgram);
	glUniformMatrix4fv(glGetUniformLocation(_debugLineProgram, "worldViewProj"), 1, 0, glm::value_ptr(worldViewProj));

	struct {
		glm::vec3 p;
		glm::vec4 c;
	} vertices[2] = {
		{ a, aColor },
	{ b, bColor },
	};

	glBindVertexArray(_debugVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, _debugVertexArray);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	// Fill in the array attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)sizeof(glm::vec3));
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_LINE_STRIP, 0, 2);
}

static void _renderPose(const glm::mat4& worldViewProj, const ovrAvatarSkinnedMeshPose& pose)
{
	glm::mat4* skinnedPoses = (glm::mat4*)alloca(sizeof(glm::mat4) * pose.jointCount);
	_computeWorldPose(pose, skinnedPoses);
	for (uint32_t i = 1; i < pose.jointCount; ++i)
	{
		int parent = pose.jointParents[i];
		_renderDebugLine(worldViewProj, glm::vec3(skinnedPoses[parent][3]), glm::vec3(skinnedPoses[i][3]), glm::vec4(1, 1, 1, 1), glm::vec4(1, 0, 0, 1));
	}
}

static void _renderSkinnedMeshPart(GLuint shader, const ovrAvatarRenderPart_SkinnedMeshRender* mesh, uint32_t visibilityMask, const glm::mat4& world, const glm::mat4& view, const glm::mat4 proj, const glm::vec3& viewPos, bool renderJoints)
{
	// If this part isn't visible from the viewpoint we're rendering from, do nothing
	if ((mesh->visibilityMask & visibilityMask) == 0)
	{
		return;
	}

	// Get the GL mesh data for this mesh's asset
	MeshData* data = (MeshData*)_assetMap[mesh->meshAssetID];

	glUseProgram(shader);

	// Apply the vertex state
	_setMeshState(shader, mesh->localTransform, data, mesh->skinnedPose, world, view, proj, viewPos);

	// Apply the material state
	_setMaterialState(shader, &mesh->materialState, nullptr);

	// Draw the mesh
	glBindVertexArray(data->vertexArray);
	glDepthFunc(GL_LESS);

	// Write to depth first for self-occlusion
	if (mesh->visibilityMask & ovrAvatarVisibilityFlag_SelfOccluding)
	{
		glDepthMask(GL_TRUE);
		glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDrawElements(GL_TRIANGLES, (GLsizei)data->elementCount, GL_UNSIGNED_SHORT, 0);
		glDepthFunc(GL_EQUAL);
	}

	// Render to color buffer
	glDepthMask(GL_FALSE);
	glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawElements(GL_TRIANGLES, (GLsizei)data->elementCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);

	if (renderJoints)
	{
		glm::mat4 local;
		_glmFromOvrAvatarTransform(mesh->localTransform, &local);
		glDepthFunc(GL_ALWAYS);
		_renderPose(proj * view * world * local, mesh->skinnedPose);
	}
}

static void _renderSkinnedMeshPartPBS(const ovrAvatarRenderPart_SkinnedMeshRenderPBS* mesh, uint32_t visibilityMask, const glm::mat4& world, const glm::mat4& view, const glm::mat4 proj, const glm::vec3& viewPos, bool renderJoints)
{
	// If this part isn't visible from the viewpoint we're rendering from, do nothing
	if ((mesh->visibilityMask & visibilityMask) == 0)
	{
		return;
	}

	// Get the GL mesh data for this mesh's asset
	MeshData* data = (MeshData*)_assetMap[mesh->meshAssetID];

	glUseProgram(_skinnedMeshPBSProgram);

	// Apply the vertex state
	_setMeshState(_skinnedMeshPBSProgram, mesh->localTransform, data, mesh->skinnedPose, world, view, proj, viewPos);

	// Apply the material state
	_setPBSState(_skinnedMeshPBSProgram, mesh->albedoTextureAssetID, mesh->surfaceTextureAssetID);

	// Draw the mesh
	glBindVertexArray(data->vertexArray);
	glDepthFunc(GL_LESS);

	// Write to depth first for self-occlusion
	if (mesh->visibilityMask & ovrAvatarVisibilityFlag_SelfOccluding)
	{
		glDepthMask(GL_TRUE);
		glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDrawElements(GL_TRIANGLES, (GLsizei)data->elementCount, GL_UNSIGNED_SHORT, 0);
		glDepthFunc(GL_EQUAL);
	}
	glDepthMask(GL_FALSE);

	// Draw the mesh
	glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawElements(GL_TRIANGLES, (GLsizei)data->elementCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);

	if (renderJoints)
	{
		glm::mat4 local;
		_glmFromOvrAvatarTransform(mesh->localTransform, &local);
		glDepthFunc(GL_ALWAYS);
		_renderPose(proj * view * world * local, mesh->skinnedPose);
	}
}

static void _renderProjector(const ovrAvatarRenderPart_ProjectorRender* projector, ovrAvatar* avatar, uint32_t visibilityMask, const glm::mat4& world, const glm::mat4& view, const glm::mat4 proj, const glm::vec3& viewPos)
{

	// Compute the mesh transform
	const ovrAvatarComponent* component = ovrAvatarComponent_Get(avatar, projector->componentIndex);
	const ovrAvatarRenderPart* renderPart = component->renderParts[projector->renderPartIndex];
	const ovrAvatarRenderPart_SkinnedMeshRender* mesh = ovrAvatarRenderPart_GetSkinnedMeshRender(renderPart);

	// If this part isn't visible from the viewpoint we're rendering from, do nothing
	if ((mesh->visibilityMask & visibilityMask) == 0)
	{
		return;
	}

	// Compute the projection matrix
	glm::mat4 projection;
	_glmFromOvrAvatarTransform(projector->localTransform, &projection);
	glm::mat4 worldProjection = world * projection;
	glm::mat4 projectionInv = glm::inverse(worldProjection);

	// Compute the mesh transform
	glm::mat4 meshWorld;
	_glmFromOvrAvatarTransform(component->transform, &meshWorld);

	// Get the GL mesh data for this mesh's asset
	MeshData* data = (MeshData*)_assetMap[mesh->meshAssetID];

	glUseProgram(_skinnedMeshProgram);

	// Apply the vertex state
	_setMeshState(_skinnedMeshProgram, mesh->localTransform, data, mesh->skinnedPose, meshWorld, view, proj, viewPos);

	// Apply the material state
	_setMaterialState(_skinnedMeshProgram, &projector->materialState, &projectionInv);

	// Draw the mesh
	glBindVertexArray(data->vertexArray);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_EQUAL);
	glDrawElements(GL_TRIANGLES, (GLsizei)data->elementCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

static void _renderAvatar(ovrAvatar* avatar, uint32_t visibilityMask, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& viewPos, bool renderJoints)
{
	// Traverse over all components on the avatar
	uint32_t componentCount = ovrAvatarComponent_Count(avatar);

	const ovrAvatarComponent* bodyComponent = nullptr;
	if (const ovrAvatarBodyComponent* body = ovrAvatarPose_GetBodyComponent(avatar))
	{
		bodyComponent = body->renderComponent;
	}

	for (uint32_t i = 0; i < componentCount; ++i)
	{
		const ovrAvatarComponent* component = ovrAvatarComponent_Get(avatar, i);

		const bool useCombinedMeshProgram = _combineMeshes && bodyComponent == component;

		// Compute the transform for this component
		glm::mat4 world;
		_glmFromOvrAvatarTransform(component->transform, &world);

		// Render each render part attached to the component
		for (uint32_t j = 0; j < component->renderPartCount; ++j)
		{
			const ovrAvatarRenderPart* renderPart = component->renderParts[j];
			ovrAvatarRenderPartType type = ovrAvatarRenderPart_GetType(renderPart);
			switch (type)
			{
			case ovrAvatarRenderPartType_SkinnedMeshRender:
				_renderSkinnedMeshPart(useCombinedMeshProgram ? _combinedMeshProgram : _skinnedMeshProgram, ovrAvatarRenderPart_GetSkinnedMeshRender(renderPart), visibilityMask, world, view, proj, viewPos, renderJoints);
				break;
			case ovrAvatarRenderPartType_SkinnedMeshRenderPBS:
				_renderSkinnedMeshPartPBS(ovrAvatarRenderPart_GetSkinnedMeshRenderPBS(renderPart), visibilityMask, world, view, proj, viewPos, renderJoints);
				break;
			case ovrAvatarRenderPartType_ProjectorRender:
				_renderProjector(ovrAvatarRenderPart_GetProjectorRender(renderPart), avatar, visibilityMask, world, view, proj, viewPos);
				break;
			}
		}
	}
}

static void _updateAvatar(
	ovrAvatar* avatar,
	float deltaSeconds,
	const ovrAvatarTransform& hmd,
	const ovrAvatarHandInputState& left,
	const ovrAvatarHandInputState& right,
	ovrMicrophone* mic,
	ovrAvatarPacket* packet,
	float* packetPlaybackTime
) {
	if (packet)
	{
		float packetDuration = ovrAvatarPacket_GetDurationSeconds(packet);
		*packetPlaybackTime += deltaSeconds;
		if (*packetPlaybackTime > packetDuration)
		{
			ovrAvatarPose_Finalize(avatar, 0.0f);
			*packetPlaybackTime = 0;
		}
		ovrAvatar_UpdatePoseFromPacket(avatar, packet, *packetPlaybackTime);
	}
	else
	{
		// If we have a mic update the voice visualization
		if (mic)
		{
			float micSamples[48000];
			size_t sampleCount = ovr_Microphone_ReadData(mic, micSamples, sizeof(micSamples) / sizeof(micSamples[0]));
			if (sampleCount > 0)
			{
				ovrAvatarPose_UpdateVoiceVisualization(_avatar, (uint32_t)sampleCount, micSamples);
			}
		}

		// Update the avatar pose from the inputs
		ovrAvatarPose_UpdateBody(avatar, hmd);
		ovrAvatarPose_UpdateHands(avatar, left, right);
	}
	ovrAvatarPose_Finalize(avatar, deltaSeconds);
}




static ovrSession _initOVR()
{
	ovrSession ovr;
	if (OVR_SUCCESS(ovr_Initialize(NULL)))
	{
		ovrGraphicsLuid luid;
		if (OVR_SUCCESS(ovr_Create(&ovr, &luid)))
		{
			return ovr;
		}
		ovr_Shutdown();
	}
	return NULL;
}

#undef main
int main()
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	
	// Attempt to initialize the Oculus SDK
	ovrSession ovr = MIRROR_ALLOW_OVR ? _initOVR() : 0;

	//vr
	//SDL_Window* window = SDL_CreateWindow("Mirror", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MIRROR_WINDOW_WIDTH, MIRROR_WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
	ovrInputState touchState;
	ovr_GetInputState(ovr, ovrControllerType_Active, &touchState);
	ovrVector2f scale = { 1.0f , 1.0f };

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(1280, 720, "Hello World", NULL, NULL);
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

	// Deadass Initialize IMGUI
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfwGL3_Init(window, true, nullptr);

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Load Fonts

	io.Fonts->AddFontFromFileTTF("dependencies/DroidSans.ttf", 16.0f);

	ImGui::GetStyle().WindowRounding = 0.0f;



	Surface surf;
	surf.Bind();

	StandardShader shd("Shaders/StandardShader.vs", "Shaders/Galaxy.fs");
	shd.VertexFileName = "Shaders/StandardShader.vs";
	shd.FragmentFileName = "Shaders/Galaxy.fs";
	shd.WindowReference = window;
	shd.Enable();

	shd.Initialize();

	std::cout << shd.GetCompileLog(ShaderType::Vertex);
	std::cout << "\n\n";
	std::cout << shd.GetCompileLog(ShaderType::Fragment);

	///////////////////////////////////////////////////////////////////////
	// TEXT EDITOR SAMPLE
	TextEditor editor;
	auto lang = TextEditor::LanguageDefinition::CPlusPlus();

	// set your own known preprocessor symbols...
	static const char* ppnames[] = { "NULL", "PM_REMOVE",
		"ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI","D3D11_SDK_VERSION", "assert" };
	// ... and their corresponding values
	static const char* ppvalues[] = {
		"#define NULL ((void*)0)",
		"#define PM_REMOVE (0x0001)",
		"Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ",
		"enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0",
		"enum D3D_FEATURE_LEVEL",
		"enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
		"#define WINAPI __stdcall",
		"#define D3D11_SDK_VERSION (7)",
		" #define assert(expression) (void)(                                                  \n"
		"    (!!(expression)) ||                                                              \n"
		"    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
		" )"
	};

	for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
	{
		TextEditor::Identifier id;
		id.mDeclaration = ppvalues[i];
		lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
	}

	// set your own identifiers
	static const char* identifiers[] = {
		"HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG","LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
		"ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
		"ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
		"IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
	static const char* idecls[] =
	{
		"typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
		"typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
		"ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
		"ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
		"IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
	for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
	{
		TextEditor::Identifier id;
		id.mDeclaration = std::string(idecls[i]);
		lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
	}
	editor.SetLanguageDefinition(lang);
	//editor.SetPalette(TextEditor::GetLightPalette());

	// error markers
	TextEditor::ErrorMarkers markers;
	markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
	markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
	editor.SetErrorMarkers(markers);

	// "breakpoint" markers
	//TextEditor::Breakpoints bpts;
	//bpts.insert(24);
	//bpts.insert(47);
	//editor.SetBreakpoints(bpts);

	static const char* fileToEdit = "shaders/OceanTest.fs";
	//	static const char* fileToEdit = "test.cpp";

	{
		std::string fil = ReadFile(fileToEdit);
		editor.SetText(fil);

	}


	// imgui]
	bool show_demo_window = true;
	bool show_another_window = false;

	ovr_GetInputState(ovr, ovrControllerType_Active, &touchState);
	ovrTrackingState trackingState = ovr_GetTrackingState(ovr, 0.0, false);





	// NEW
	// Create the microphone for voice effects
	ovrMicrophoneHandle mic = ovr_Microphone_Create();
	if (mic)
	{
		ovr_Microphone_Start(mic);
	}

	// If we're in VR mode, initialize the swap chain
	ovrHmdDesc hmdDesc;
	GLuint mirrorFBO = 0;
	ovrTextureSwapChain eyeSwapChains[2];
	GLuint eyeFrameBuffers[2];
	GLuint eyeDepthBuffers[2];
	ovrSizei eyeSizes[2];
	if (ovr)
	{
		// Get the buffer size we need for rendering
		hmdDesc = ovr_GetHmdDesc(ovr);
		for (int eye = 0; eye < 2; ++eye)
		{
			eyeSizes[eye] = ovr_GetFovTextureSize(ovr, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);

			// Create the swap chain
			ovrTextureSwapChainDesc desc;
			memset(&desc, 0, sizeof(desc));
			desc.Type = ovrTexture_2D;
			desc.ArraySize = 1;
			desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
			desc.Width = eyeSizes[eye].w;
			desc.Height = eyeSizes[eye].h;
			desc.MipLevels = 1;
			desc.SampleCount = 1;
			desc.StaticImage = ovrFalse;
			ovr_CreateTextureSwapChainGL(ovr, &desc, &eyeSwapChains[eye]);

			int length = 0;
			ovr_GetTextureSwapChainLength(ovr, eyeSwapChains[eye], &length);
			for (int i = 0; i < length; ++i)
			{
				GLuint chainTexId;
				ovr_GetTextureSwapChainBufferGL(ovr, eyeSwapChains[eye], i, &chainTexId);
				glBindTexture(GL_TEXTURE_2D, chainTexId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			glGenFramebuffers(1, &eyeFrameBuffers[eye]);

			glGenTextures(1, &eyeDepthBuffers[eye]);
			glBindTexture(GL_TEXTURE_2D, eyeDepthBuffers[eye]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, eyeSizes[eye].w, eyeSizes[eye].h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
		}

		// Create mirror buffer
		ovrMirrorTextureDesc mirrorDesc;
		memset(&mirrorDesc, 0, sizeof(mirrorDesc));
		mirrorDesc.Width = MIRROR_WINDOW_WIDTH;
		mirrorDesc.Height = MIRROR_WINDOW_HEIGHT;
		mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

		ovrMirrorTexture mirrorTexture;
		ovr_CreateMirrorTextureGL(ovr, &mirrorDesc, &mirrorTexture);

		GLuint mirrorTextureID;
		ovr_GetMirrorTextureBufferGL(ovr, mirrorTexture, &mirrorTextureID);

		glGenFramebuffers(1, &mirrorFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureID, 0);
		glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
	if (ovr)
	{
		ovr_SetTrackingOriginType(ovr, ovrTrackingOrigin_FloorLevel);
		ovr_RecenterTrackingOrigin(ovr);
	}


	// EEE

	// Run the main loop
	bool recording = false;
	bool controllersVisible = false;
	bool customBasePosition = false;
	bool renderJoints = false;
	bool freezePose = false;
	int capabilities = ovrAvatarCapability_All;
	bool running = true;
	long long frameIndex = 0;
	ovrAvatarPacket* playbackPacket = nullptr;
	float playbackTime = 0;
	std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
	uint64_t testUserID = 0;

	float clear_color[3] = { 0, 0, 0 };

	//bool running = true;
	while (running)
	{
		static char fragFile[128] = "";

		// Clear the screen to black
		glClearColor(1.0f, 0.0f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// IMGUI
		ImGui_ImplGlfwGL3_NewFrame(window, gtime);
		ImGui::GetStyle().Alpha = 0.65f;

		//VR
		ovr_GetInputState(ovr, ovrControllerType_Active, &touchState);
		ovrTrackingState trackingState = ovr_GetTrackingState(ovr, 0.0, false);

		shd.Variables[0] = touchState.Thumbstick[0].x;
		shd.Variables[1] = touchState.Thumbstick[0].y;
		shd.Variables[2] = touchState.Thumbstick[1].x;
		shd.Variables[3] = touchState.Thumbstick[1].y;

		shd.Triggers[0] = touchState.IndexTrigger[0];
		shd.Triggers[1] = touchState.HandTrigger[0];
		shd.Triggers[2] = touchState.IndexTrigger[1];
		shd.Triggers[3] = touchState.HandTrigger[1];


		auto cpos = editor.GetCursorPosition();
		ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
		ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save"))
				{
					auto textToSave = editor.GetText();

					std::ofstream out(fragFile);
					out << textToSave;
					out.close();
					/// save text....
				}
				if (ImGui::MenuItem("Open"))
				{
					static const char* fileToEdit = fragFile;
					//	static const char* fileToEdit = "test.cpp";

					{
						std::string fil = ReadFile(fragFile);
						editor.SetText(fil);

					}
				}
				if (ImGui::MenuItem("Quit", "Alt-F4"))
					break;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				bool ro = editor.IsReadOnly();
				if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
					editor.SetReadOnly(ro);
				ImGui::Separator();

				if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
					editor.Undo();
				if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
					editor.Redo();

				ImGui::Separator();

				if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
					editor.Copy();
				if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
					editor.Cut();
				if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
					editor.Delete();
				if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
					editor.Paste();

				ImGui::Separator();

				if (ImGui::MenuItem("Select all", nullptr, nullptr))
					editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Dark palette"))
					editor.SetPalette(TextEditor::GetDarkPalette());
				if (ImGui::MenuItem("Light palette"))
					editor.SetPalette(TextEditor::GetLightPalette());
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
			editor.IsOverwrite() ? "Ovr" : "Ins",
			editor.CanUndo() ? "*" : " ",
			editor.GetLanguageDefinition().mName.c_str(), fileToEdit);

		editor.Render("TextEditor");
		ImGui::End();

		{


			static float f = 0.0f;
			static int counter = 0;
			ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::Text("Left Input X values = %f", touchState.Thumbstick[0].x * 100);
			ImGui::Text("Left Input Y values = %f", touchState.Thumbstick[0].y * 100);
			ImGui::Text("Right Input X values = %f", touchState.Thumbstick[1].x * 100);
			ImGui::Text("Right Input Y values = %f", touchState.Thumbstick[1].y * 100);

			ImGui::NewLine();

			ImGui::Text("Left Index Trigger = %f", touchState.IndexTrigger[0] * 100);
			ImGui::Text("Left Hand Trigger = %f", touchState.HandTrigger[0] * 100);
			ImGui::Text("Right Index Trigger = %f", touchState.IndexTrigger[1] * 100);
			ImGui::Text("Right Hand Trigger = %f", touchState.HandTrigger[1] * 100);

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
				counter++;

			ImGui::NewLine();
			ImGui::NewLine();

			ImGui::InputText("pixel shader filename", fragFile, IM_ARRAYSIZE(fragFile));

			if (ImGui::Button("Recompile"))
			{
				shd.FragmentFileName = std::string(fragFile);

				shd.Recompile();
				shd.Enable();
				shd.Initialize();

				std::cout << "\n\nNEW SHADER\n";
				std::cout << shd.GetCompileLog(ShaderType::Vertex);
				std::cout << "\n\n";
				std::cout << shd.GetCompileLog(ShaderType::Fragment);
				std::cout << "\n";
			}

			//ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			//ImGui::SetWindowFontScale(2.5f);

		}


		// STOP

		//io.FontGlobalScale = 2.0f;

		shd.Enable();
		shd.Update();

		//glBindVertexArray(vao);
		// Draw a triangle from the 3 vertices
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// DRAW OVER IT
		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());



		// Compute how much time has elapsed since the last frame
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<float> deltaTime = currentTime - lastTime;
		float deltaSeconds = deltaTime.count();
		lastTime = currentTime;
		_elapsedSeconds += deltaSeconds;

		shd.Enable();

		// AAAAAAAAA
		if (ovr)
		{

			// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(ovr, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(ovr, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

			// Get eye poses, feeding in correct IPD offset
			ovrPosef                  eyeRenderPose[2];
			ovrVector3f               hmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset, eyeRenderDesc[1].HmdToEyeOffset };
			double sensorSampleTime;
			ovr_GetEyePoses(ovr, frameIndex, ovrTrue, hmdToEyeOffset, eyeRenderPose, &sensorSampleTime);

			// If the avatar is initialized, update it
			if (_avatar)
			{
				// Convert the OVR inputs into Avatar SDK inputs
				ovrInputState touchState;
				ovr_GetInputState(ovr, ovrControllerType_Active, &touchState);
				ovrTrackingState trackingState = ovr_GetTrackingState(ovr, 0.0, false);

				glm::vec3 hmdP = _glmFromOvrVector(trackingState.HeadPose.ThePose.Position);
				glm::quat hmdQ = _glmFromOvrQuat(trackingState.HeadPose.ThePose.Orientation);
				glm::vec3 leftP = _glmFromOvrVector(trackingState.HandPoses[ovrHand_Left].ThePose.Position);
				glm::quat leftQ = _glmFromOvrQuat(trackingState.HandPoses[ovrHand_Left].ThePose.Orientation);
				glm::vec3 rightP = _glmFromOvrVector(trackingState.HandPoses[ovrHand_Right].ThePose.Position);
				glm::quat rightQ = _glmFromOvrQuat(trackingState.HandPoses[ovrHand_Right].ThePose.Orientation);

				ovrAvatarTransform hmd;
				_ovrAvatarTransformFromGlm(hmdP, hmdQ, glm::vec3(1.0f), &hmd);

				ovrAvatarTransform left;
				_ovrAvatarTransformFromGlm(leftP, leftQ, glm::vec3(1.0f), &left);

				ovrAvatarTransform right;
				_ovrAvatarTransformFromGlm(rightP, rightQ, glm::vec3(1.0f), &right);

				ovrAvatarHandInputState inputStateLeft;
				_ovrAvatarHandInputStateFromOvr(left, touchState, ovrHand_Left, &inputStateLeft);

				ovrAvatarHandInputState inputStateRight;
				_ovrAvatarHandInputStateFromOvr(right, touchState, ovrHand_Right, &inputStateRight);

				_updateAvatar(_avatar, deltaSeconds, hmd, inputStateLeft, inputStateRight, mic, playbackPacket, &playbackTime);
			}

			// Render each eye
			for (int eye = 0; eye < 2; ++eye)
			{
				// Switch to eye render target
				int curIndex;
				GLuint curTexId;
				ovr_GetTextureSwapChainCurrentIndex(ovr, eyeSwapChains[eye], &curIndex);
				ovr_GetTextureSwapChainBufferGL(ovr, eyeSwapChains[eye], curIndex, &curTexId);

				glBindFramebuffer(GL_FRAMEBUFFER, eyeFrameBuffers[eye]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, eyeDepthBuffers[eye], 0);

				glViewport(0, 0, eyeSizes[eye].w, eyeSizes[eye].h);
				glDepthMask(GL_TRUE);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glEnable(GL_FRAMEBUFFER_SRGB);

				surf.Bind();
				glDrawArrays(GL_TRIANGLES, 0, 6);

				ovrVector3f eyePosition = eyeRenderPose[eye].Position;
				ovrQuatf eyeOrientation = eyeRenderPose[eye].Orientation;
				glm::quat glmOrientation = _glmFromOvrQuat(eyeOrientation);
				glm::vec3 eyeWorld = _glmFromOvrVector(eyePosition);
				glm::vec3 eyeForward = glmOrientation * glm::vec3(0, 0, -1);
				glm::vec3 eyeUp = glmOrientation * glm::vec3(0, 1, 0);
				glm::mat4 view = glm::lookAt(eyeWorld, eyeWorld + eyeForward, eyeUp);

				ovrMatrix4f ovrProjection = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.01f, 1000.0f, ovrProjection_None);
				glm::mat4 proj(
					ovrProjection.M[0][0], ovrProjection.M[1][0], ovrProjection.M[2][0], ovrProjection.M[3][0],
					ovrProjection.M[0][1], ovrProjection.M[1][1], ovrProjection.M[2][1], ovrProjection.M[3][1],
					ovrProjection.M[0][2], ovrProjection.M[1][2], ovrProjection.M[2][2], ovrProjection.M[3][2],
					ovrProjection.M[0][3], ovrProjection.M[1][3], ovrProjection.M[2][3], ovrProjection.M[3][3]
				);

				// If we have the avatar and have finished loading assets, render it
				if (_avatar && !_loadingAssets && !_waitingOnCombinedMesh)
				{
					_renderAvatar(_avatar, ovrAvatarVisibilityFlag_FirstPerson, view, proj, eyeWorld, renderJoints);

					glm::vec4 reflectionPlane = glm::vec4(0.0, 0.0, -1.0, 0.0);
					glm::mat4 reflection = _computeReflectionMatrix(reflectionPlane);

					glFrontFace(GL_CW);
					_renderAvatar(_avatar, ovrAvatarVisibilityFlag_ThirdPerson, view * reflection, proj, glm::vec3(reflection * glm::vec4(eyeWorld, 1.0f)), renderJoints);
					glFrontFace(GL_CCW);
				}

				// Unbind the eye buffer
				glBindFramebuffer(GL_FRAMEBUFFER, eyeFrameBuffers[eye]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

				// Commit changes to the textures so they get picked up frame
				ovr_CommitTextureSwapChain(ovr, eyeSwapChains[eye]);
			}

			// Prepare the layers
			ovrLayerEyeFov layerDesc;
			memset(&layerDesc, 0, sizeof(layerDesc));
			layerDesc.Header.Type = ovrLayerType_EyeFov;
			layerDesc.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.
			for (int eye = 0; eye < 2; ++eye)
			{
				layerDesc.ColorTexture[eye] = eyeSwapChains[eye];
				layerDesc.Viewport[eye].Size = eyeSizes[eye];
				layerDesc.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				layerDesc.RenderPose[eye] = eyeRenderPose[eye];
				layerDesc.SensorSampleTime = sensorSampleTime;
			}

			ovrLayerHeader* layers = &layerDesc.Header;
			ovr_SubmitFrame(ovr, frameIndex, NULL, &layers, 1);

			ovrSessionStatus sessionStatus;
			ovr_GetSessionStatus(ovr, &sessionStatus);
			if (sessionStatus.ShouldQuit)
				running = false;
			if (sessionStatus.ShouldRecenter)
				ovr_RecenterTrackingOrigin(ovr);

			// Blit mirror texture to back buffer
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, MIRROR_WINDOW_HEIGHT, MIRROR_WINDOW_WIDTH, 0, 0, 0, MIRROR_WINDOW_WIDTH, MIRROR_WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		// Render to 2D viewport
		else
		{
			glDepthMask(GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glm::vec3 eyePos = glm::vec3(0, 1.0f, -2.5f);
			glm::vec3 eyeTarget = glm::vec3(0, 1.0f, 0.0f);
			glm::mat4 view = glm::lookAt(eyePos, eyeTarget, glm::vec3(0, 1, 0));
			glm::mat4 proj = glm::perspectiveFov(glm::radians(45.0f), (float)MIRROR_WINDOW_WIDTH, (float)MIRROR_WINDOW_HEIGHT, 0.01f, 100000.0f);
			if (_avatar && !_loadingAssets)
			{
				// Compute the total elapsed time so that we can animate a rotation of the avatar
				static bool rotate = true;
				static float rotateTheta;
				if (rotate)
				{
					rotateTheta += deltaSeconds;
					while (rotateTheta > glm::radians(360.0f))
					{
						rotateTheta -= glm::radians(360.0f);
					}
				}

				// Compute poses for each of the components
				glm::quat orientation = glm::quat(glm::vec3(0, rotateTheta, 0));
				glm::vec3 bodyPosition = orientation * glm::vec3(0, 1.75f, 0.25f);
				glm::vec3 handLeftPosition = orientation * glm::vec3(-0.25, 1.5f, -0.25);
				glm::vec3 handRightPosition = orientation * glm::vec3(0.25, 1.5f, -0.25);

				ovrAvatarTransform bodyPose, handLeftPose, handRightPose;
				_ovrAvatarTransformFromGlm(bodyPosition, orientation, glm::vec3(1, 1, 1), &bodyPose);
				_ovrAvatarTransformFromGlm(handLeftPosition, orientation, glm::vec3(1, 1, 1), &handLeftPose);
				_ovrAvatarTransformFromGlm(handRightPosition, orientation, glm::vec3(1, 1, 1), &handRightPose);

				// Synthesize some input
				ovrInputState inputState;
				memset(&inputState, 0, sizeof(inputState));
				inputState.ControllerType = ovrControllerType_Touch;
				inputState.Touches |= ovrTouch_LIndexPointing;
				inputState.Touches |= ovrTouch_RThumbUp;
				inputState.HandTrigger[ovrHand_Left] = 0.5;
				inputState.HandTrigger[ovrHand_Right] = 1.0;

				ovrAvatarHandInputState leftInputState;
				_ovrAvatarHandInputStateFromOvr(handLeftPose, inputState, ovrHand_Left, &leftInputState);

				ovrAvatarHandInputState rightInputState;
				_ovrAvatarHandInputStateFromOvr(handRightPose, inputState, ovrHand_Right, &rightInputState);

				_updateAvatar(_avatar, deltaSeconds, bodyPose, leftInputState, rightInputState, mic, playbackPacket, &playbackTime);

				// Render the avatar
				_renderAvatar(_avatar, ovrAvatarVisibilityFlag_ThirdPerson, view, proj, eyePos, renderJoints);
			}
		}





		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	//glDeleteProgram(shaderProgram);f
	//glDeleteShader(fragmentShader);
	//glDeleteShader(vertexShader);

	//glDeleteBuffers(1, &vbo);

	//glDeleteVertexArrays(1, &vao);


	return 0;
}