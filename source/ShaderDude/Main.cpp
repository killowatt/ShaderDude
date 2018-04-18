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

bool UIManager::g_MouseJustPressed[3] = { false, false, false };

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
	// Initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Initialize OculusVR
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
		return -1;

	ovrSession session;
	ovrGraphicsLuid luid;
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		ovr_Shutdown();
		return -1;
	}

	ovrHmdDesc desc = ovr_GetHmdDesc(session);
	ovrSizei resolution = desc.Resolution;

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
	bool show_demo_window = false;
	bool show_another_window = false;

	bool running = true;
	while (!glfwWindowShouldClose(window))
	{
		static char fragFile[128] = "";

		// Clear the screen to black
		glClearColor(1.0f, 0.0f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// IMGUI
		ui.ImGui_ImplGlfwGL3_NewFrame(window, ui.gtime);
		ImGui::GetStyle().Alpha = 0.65f;

		//VR
		//ovr_GetInputState(ovr, ovrControllerType_Active, &touchState);
		//ovrTrackingState trackingState = ovr_GetTrackingState(ovr, 0.0, false);

		//shd.Variables[0] = touchState.Thumbstick[0].x;
		//shd.Variables[1] = touchState.Thumbstick[0].y;
		//shd.Variables[2] = touchState.Thumbstick[1].x;
		//shd.Variables[3] = touchState.Thumbstick[1].y;

		//shd.Triggers[0] = touchState.IndexTrigger[0];
		//shd.Triggers[1] = touchState.HandTrigger[0];
		//shd.Triggers[2] = touchState.IndexTrigger[1];
		//shd.Triggers[3] = touchState.HandTrigger[1];


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


		if (show_another_window)
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

			ImGui::Checkbox("Show VR", &show_demo_window);      // Edit bools storing our windows open/close state
			ImGui::Checkbox("Show Help", &show_another_window);

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
				/*				ImGui::Text("Analog Stick Values");
				ImGui::Text("Left Input X values = %f", touchState.Thumbstick[0].x * 100);
				ImGui::Text("Left Input Y values = %f", touchState.Thumbstick[0].y * 100);
				ImGui::Text("Right Input X values = %f", touchState.Thumbstick[1].x * 100);
				ImGui::Text("Right Input Y values = %f", touchState.Thumbstick[1].y * 100);

				ImGui::NewLine();

				ImGui::Text("Trigger Values");
				ImGui::Text("Left Index Trigger = %f", touchState.IndexTrigger[0] * 100);
				ImGui::Text("Left Hand Trigger = %f", touchState.HandTrigger[0] * 100);
				ImGui::Text("Right Index Trigger = %f", touchState.IndexTrigger[1] * 100);
				ImGui::Text("Right Hand Trigger = %f", touchState.HandTrigger[1] * 100)*/;

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
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// DRAW OVER IT
		ImGui::Render();
		ui.ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());


		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	// Deinit Oculus
	ovr_Destroy(session);
	ovr_Shutdown();

	return 0;
}