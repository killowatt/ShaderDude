// Ex code below

// Link statically with GLEW

// Headers
#include "Core.h"

#include <fstream>
#include <iostream>
#include <exception>

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Shaders\StandardShader.h>
#include <Graphics\Surface.h>

#include "UI/ImGui/imgui.h"
#include "GLFW\glfw3native.h"

#include "UI/TextEditor.h"
#include "UI/UserInterface.h"

// Oculus
#include <LibOVR/OVR_CAPI.h>
#include <LibOVR/OVR_CAPI_GL.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

bool UIManager::g_MouseJustPressed[3] = { false, false, false };

// transform funcz
static glm::vec3 _glmFromOvrVector(const ovrVector3f& ovrVector)
{
	return glm::vec3(ovrVector.x, ovrVector.y, ovrVector.z);
}

static glm::quat _glmFromOvrQuat(const ovrQuatf& ovrQuat)
{
	return glm::quat(ovrQuat.w, ovrQuat.x, ovrQuat.y, ovrQuat.z);
}

//const int32 mirrorWidth = 1920 / 3;
//const int32 mirrorHeight = 1080 / 3;
const int32 mirrorWidth = 800 / 4;
const int32 mirrorHeight = 600 / 4;

uint64 frameIndex;
int main()
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(1280, 720, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Initialize OculusVR
	bool vrCapable = true;

	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
	{
		std::cout << "Couldn't initialize OVR\n";
		vrCapable = false;
	}

	ovrSession session;
	ovrGraphicsLuid luid;
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		std::cout << "Couldn't create VR session\n";
		ovr_Shutdown();
		vrCapable = false;
	}

	// Controller Init before we do anything crazy
	ovrInputState touchState;
	ovr_GetInputState(session, ovrControllerType_Active, &touchState);

	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	ovrSizei resolution = hmdDesc.Resolution;

	//// Texture Swapchain Init
	//ovrSizei recommendedTex0Size = ovr_GetFovTextureSize(session, ovrEye_Left,
	//	desc.DefaultEyeFov[0], 1.0f);
	//ovrSizei recommendedTex1Size = ovr_GetFovTextureSize(session, ovrEye_Right,
	//	desc.DefaultEyeFov[1], 1.0f);

	//ovrSizei bufferSize;
	//bufferSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
	//bufferSize.h = max(recommendedTex1Size.h, recommendedTex0Size.h);

	// Mirror/Swapchain init
	GLuint mirrorFBO = 0;
	ovrTextureSwapChain textureSwapChains[2];
	GLuint eyeFrameBuffers[2];
	GLuint eyeDepthBuffers[2];
	ovrSizei eyeSizes[2];

	ovrSizei bufferSize;
	bufferSize.w = 50;
	bufferSize.h = 50;
	if (session)
	{
		for (int eye = 0; eye < 2; eye++)
		{
			eyeSizes[eye] = ovr_GetFovTextureSize(session, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);

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

			ovr_CreateTextureSwapChainGL(session, &desc, &textureSwapChains[eye]);

			int length = 0;
			ovr_GetTextureSwapChainLength(session, textureSwapChains[eye], &length);
			for (int i = 0; i < length; ++i)
			{
				GLuint chainTexId;
				ovr_GetTextureSwapChainBufferGL(session, textureSwapChains[eye], i, &chainTexId);
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

		// calc buf size
		bufferSize.w = eyeSizes[0].w + eyeSizes[1].w;
		bufferSize.h = std::max(eyeSizes[0].h, eyeSizes[1].h);

		// Create mirror buffer
		ovrMirrorTextureDesc mirrorDesc;
		memset(&mirrorDesc, 0, sizeof(mirrorDesc));
		mirrorDesc.Width = bufferSize.w;
		mirrorDesc.Height = bufferSize.h;
		mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

		ovrMirrorTexture mirrorTexture;
		ovr_CreateMirrorTextureGL(session, &mirrorDesc, &mirrorTexture);

		GLuint mirrorTextureID;
		ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &mirrorTextureID);

		glGenFramebuffers(1, &mirrorFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureID, 0);
		glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
	if (session)
	{
		ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
		ovr_RecenterTrackingOrigin(session);
	}

	//// OCULUS CONST
	//// Initialize VR structures, filling out description.
	//ovrEyeRenderDesc eyeRenderDesc[2];
	//ovrPosef      hmdToEyeViewPose[2];
	//ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	//eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	//eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
	//hmdToEyeViewPose[0] = eyeRenderDesc[0].HmdToEyePose;
	//hmdToEyeViewPose[1] = eyeRenderDesc[1].HmdToEyePose;

	//// Initialize our single full screen Fov layer.
	//ovrLayerEyeFov layer;
	//layer.Header.Type = ovrLayerType_EyeFov;
	//layer.Header.Flags = 0;
	//layer.ColorTexture[0] = textureSwapChain;
	//layer.ColorTexture[1] = textureSwapChain;
	//layer.Fov[0] = eyeRenderDesc[0].Fov;
	//layer.Fov[1] = eyeRenderDesc[1].Fov;

	//layer.Viewport[0].Pos.x = 0;
	//layer.Viewport[0].Pos.y = 0;
	//layer.Viewport[0].Size.w = bufferSize.w / 2;
	//layer.Viewport[0].Size.h = bufferSize.h;

	//layer.Viewport[1].Pos.x = bufferSize.w / 2;
	//layer.Viewport[1].Pos.y = 0;
	//layer.Viewport[1].Size.w = bufferSize.w / 2;
	//layer.Viewport[1].Size.h = bufferSize.h;
	//// ld.RenderPose and ld.SensorSampleTime are updated later per frame.

	//if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	//{
	//	std::cout << "Oculus framebuffer appears to be incomplete.\n";
	//}


	// Initialize ImGui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	UIManager ui;
	ui.ImGui_ImplGlfwGL3_Init(window, true, nullptr);

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("dependencies/DroidSans.ttf", 16.0f);

	// Set our style
	ImGui::GetStyle().WindowRounding = 0.0f;



	// Initialize our shader rendering surface
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

	editor.SetLanguageDefinition(lang);
	//editor.SetPalette(TextEditor::GetLightPalette());

	// error markers
	TextEditor::ErrorMarkers markers;
	//markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
	//markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
	//editor.SetErrorMarkers(markers);

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
	bool vrEnabled = false;
	bool showHelp = false;

	bool running = true;
	while (!glfwWindowShouldClose(window))
	{
		static char fragFile[128] = "";

		// Clear the screen
		glClearColor(1.0f, 0.0f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		// Get Controller Data
		ovr_GetInputState(session, ovrControllerType_Active, &touchState);
		ovrTrackingState trackingState = ovr_GetTrackingState(session, 0.0, false);

		shd.Variables[0] = touchState.Thumbstick[0].x;
		shd.Variables[1] = touchState.Thumbstick[0].y;
		shd.Variables[2] = touchState.Thumbstick[1].x;
		shd.Variables[3] = touchState.Thumbstick[1].y;

		shd.Triggers[0] = touchState.IndexTrigger[0];
		shd.Triggers[1] = touchState.HandTrigger[0];
		shd.Triggers[2] = touchState.IndexTrigger[1];
		shd.Triggers[3] = touchState.HandTrigger[1];


		// IMGUI
		ui.ImGui_ImplGlfwGL3_NewFrame(window, ui.gtime);
		ImGui::GetStyle().Alpha = 0.65f;

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


		if (showHelp)
		{

			ImGui::Begin("Help");
			ImGui::SetWindowSize(ImVec2(320, 160), ImGuiCond_FirstUseEver);
			ImGui::Text("Coming Soon");
			ImGui::End();
		}
		{
			ImGui::Begin("Control Panel");
			ImGui::SetWindowSize(ImVec2(512, 768), ImGuiCond_FirstUseEver);
			static float f = 0.0f;
			static int counter = 0;
			ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
																	//ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
																	//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			ImGui::Checkbox("Show VR", &vrEnabled);      // Edit bools storing our windows open/close state
			ImGui::Checkbox("Show Help", &showHelp);

			ImGui::InputText("Shader FileName", fragFile, IM_ARRAYSIZE(fragFile));

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

			ImGui::NewLine();
			ImGui::NewLine();

			if (ImGui::CollapsingHeader("Technical Info"))
			{
				ImGui::Text("Analog Stick Values");
				ImGui::Text("Left Input X values = %f", touchState.Thumbstick[0].x * 100);
				ImGui::Text("Left Input Y values = %f", touchState.Thumbstick[0].y * 100);
				ImGui::Text("Right Input X values = %f", touchState.Thumbstick[1].x * 100);
				ImGui::Text("Right Input Y values = %f", touchState.Thumbstick[1].y * 100);

				ImGui::NewLine();

				ImGui::Text("Trigger Values");
				ImGui::Text("Left Index Trigger = %f", touchState.IndexTrigger[0] * 100);
				ImGui::Text("Left Hand Trigger = %f", touchState.HandTrigger[0] * 100);
				ImGui::Text("Right Index Trigger = %f", touchState.IndexTrigger[1] * 100);
				ImGui::Text("Right Hand Trigger = %f", touchState.HandTrigger[1] * 100);

			ImGui::NewLine();

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			}


			ImGui::End();
			//if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
			//	counter++;


			//ImGui::SameLine();
			//ImGui::Text("counter = %d", counter);

			//ImGui::SetWindowFontScale(2.5f);

		}

		// STOP

		//io.FontGlobalScale = 2.0f;


		shd.Enable();
		shd.Update();

		//glBindVertexArray(vao);
		// Draw a triangle from the 3 vertices
		glBindVertexArray(surf.GetVertexArray());
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// DRAW OVER IT
		ImGui::Render();
		ui.ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());


		if (session && vrEnabled)
		{
			// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

			ovrPosef eyeRenderPose[2];
			ovrPosef hmdToEyeOffset[2] = 
			{ eyeRenderDesc[0].HmdToEyePose, eyeRenderDesc[1].HmdToEyePose };

			double sensorSampleTime;
			ovr_GetEyePoses(session, frameIndex, ovrTrue, hmdToEyeOffset, eyeRenderPose,
				&sensorSampleTime);

			// Render each eye
			for (int eye = 0; eye < 2; ++eye)
			{
				// Switch to eye render target
				int curIndex;
				GLuint curTexId;
				ovr_GetTextureSwapChainCurrentIndex(session, textureSwapChains[eye],
					&curIndex);
				ovr_GetTextureSwapChainBufferGL(session, textureSwapChains[eye],
					curIndex, &curTexId);

				glBindFramebuffer(GL_FRAMEBUFFER, eyeFrameBuffers[eye]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, eyeDepthBuffers[eye], 0);

				glViewport(0, 0, eyeSizes[eye].w, eyeSizes[eye].h);
				glDepthMask(GL_TRUE);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				// This changes the gamma of our scene in a way we dislike.
				// Uncomment and the gamma will change back.
				//glEnable(GL_FRAMEBUFFER_SRGB);

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

				// Unbind the eye buffer
				glBindFramebuffer(GL_FRAMEBUFFER, eyeFrameBuffers[eye]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

				// Commit changes to the textures so they get picked up frame
				ovr_CommitTextureSwapChain(session, textureSwapChains[eye]);
			}

			// Prepare the layers
			ovrLayerEyeFov layerDesc;
			memset(&layerDesc, 0, sizeof(layerDesc));
			layerDesc.Header.Type = ovrLayerType_EyeFov;
			layerDesc.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.
			for (int eye = 0; eye < 2; eye++)
			{
				layerDesc.ColorTexture[eye] = textureSwapChains[eye];
				layerDesc.Viewport[eye].Size = eyeSizes[eye];
				layerDesc.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				layerDesc.RenderPose[eye] = eyeRenderPose[eye];
				layerDesc.SensorSampleTime = sensorSampleTime;
			}

			ovrLayerHeader* layers = &layerDesc.Header;
			ovr_SubmitFrame(session, frameIndex, NULL, &layers, 1);

			ovrSessionStatus sessionStatus;
			ovr_GetSessionStatus(session, &sessionStatus);
			if (sessionStatus.ShouldQuit)
				running = false;
			if (sessionStatus.ShouldRecenter)
				ovr_RecenterTrackingOrigin(session);

			// Blit mirror texture to back buffer
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			//glBlitFramebuffer(0, mirrorWidth, mirrorHeight,
			//	0, 0, 0, mirrorWidth, mirrorHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBlitFramebuffer(0, bufferSize.h, bufferSize.w,
				0, 0, 0, bufferSize.w / 5, bufferSize.h / 5, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}



		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);

		frameIndex++;
	}

	// Deinit Oculus
	ovr_Destroy(session);
	ovr_Shutdown();

	return 0;
}