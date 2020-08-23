#include "myUI.hpp"
#include "myTransfer.hpp"

#include <imgui_internal.h>

#include <Windows.h>
#include <shellapi.h>

#include <chrono>
#include <thread>
#include <mutex>

extern std::mutex GLOB_LOCK;
extern bool GLOB_PROGRAM_EXIT;
extern bool GLOB_CONNECTED;
extern TransferManager* myTranManager;
extern std::mutex lock_Transfer;

UIManager::UIManager() : myConnectInfo()
{
	if (!initialize_sdl()) return;
	if (!initialize_glew())
	{
		quit_sdl();
		return;
	}
	initialize_imgui();
	initialized = true;
}

UIManager::~UIManager()
{
	quit_imgui();
	quit_sdl();
}

void UIManager::loop()
{
	bool done = false;
	Uint32 tNow = SDL_GetTicks();
	Uint32 tPrev = SDL_GetTicks();
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			// process events
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type)
			{
			case SDL_QUIT:
				done = true;
				GLOB_LOCK.lock();
				GLOB_PROGRAM_EXIT = true;
				GLOB_LOCK.unlock();
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(myWindow))
				{
					done = true;
					GLOB_LOCK.lock();
					GLOB_PROGRAM_EXIT = true;
					GLOB_LOCK.unlock();
				}
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					done = true;
					GLOB_LOCK.lock();
					GLOB_PROGRAM_EXIT = true;
					GLOB_LOCK.unlock();
				}
				break;
			}
		}
		// start imgui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(myWindow);
		ImGui::NewFrame();
		// draw actual UI
		draw_UI();
		// end frame
		ImGui::Render();
		glViewport(0, 0, window_size_width, window_size_height);
		glClearColor(window_color_bg.x, window_color_bg.y,
			window_color_bg.z, window_color_bg.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(myWindow);
		// limit fps
		fps_control(tPrev, tNow);
	}
}

void UIManager::draw_UI()
{
	// windows choose connection type
	ImGui::PushStyleColor(ImGuiCol_WindowBg, window_color_cf);

	ImGui::SetNextWindowPos(ImVec2(0.04f * window_size_width, 0.04f * window_size_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(0.44f * window_size_width, 0.3f * window_size_height), ImGuiCond_Always);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::PushStyleColor(ImGuiCol_Text, window_color_font_title);
	ImGui::Begin("Configuration", NULL, flags);
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_Text, window_color_font_inside);

	ImGui::Text("Choose your connection type:");
	ImGui::PushStyleColor(ImGuiCol_CheckMark, window_color_radio_bt);
	lock_Transfer.lock();
	if (myTranManager)
	{
		// if already radio button lock required, then disable radio buttons
		// different from GLOB_CONNECTED, since start button request may not result in a connected status
		// but radio button should always be locked after a start button request to avoid error
		if (GLOB_CONNECTED)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		}
		ImGui::RadioButton("Wifi", reinterpret_cast<int*>(&myTranManager->transfer_type), 1);
		ImGui::RadioButton("Bluetooth", reinterpret_cast<int*>(&myTranManager->transfer_type), 0);
		if(GLOB_CONNECTED)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}
	lock_Transfer.unlock();
	ImGui::PopStyleColor();

	ImGui::PopStyleColor();
	ImGui::End();

	// buttons
	ImGui::SetNextWindowPos(ImVec2(0.04f * window_size_width, 0.38f * window_size_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(0.44f * window_size_width, 0.58f * window_size_height), ImGuiCond_Always);
	flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::PushStyleColor(ImGuiCol_Text, window_color_font_title);
	ImGui::Begin(" ", NULL, flags);
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_Text, window_color_font_inside);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, window_color_bt);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, window_color_bt_hover);
	ImGui::SetCursorPos(ImVec2(0.07f * window_size_width, 0.05f * window_size_height));
	// if already connected, then disable the start service button
	if (GLOB_CONNECTED)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}
	if (ImGui::Button("Start Service", ImVec2(0.3f * window_size_width, 0.12f * window_size_height)))
	{
		if (lock_Transfer.try_lock())
		{
			if(myTranManager)
				myTranManager->start_requested = true;
			lock_Transfer.unlock();
		}
	}
	if (GLOB_CONNECTED)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
	ImGui::SetCursorPos(ImVec2(0.07f * window_size_width, 0.22f * window_size_height));
	if (ImGui::Button("Stop Service", ImVec2(0.3f * window_size_width, 0.12f * window_size_height)))
	{
		if (lock_Transfer.try_lock())
		{
			if (myTranManager)
				myTranManager->stop_requested = true;
			lock_Transfer.unlock();
		}
	}
	ImGui::SetCursorPos(ImVec2(0.07f * window_size_width, 0.39f * window_size_height));
	if (ImGui::Button("Help", ImVec2(0.3f * window_size_width, 0.12f * window_size_height)))
	{
		// help button will launch github website of this project
		ShellExecuteA(NULL, NULL, "https://github.com/teamclouday/AndroidTrackpad/tree/master/Windows", NULL, NULL, SW_SHOW);
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	ImGui::PopStyleColor();
	ImGui::End();

	// connection information
	ImGui::SetNextWindowPos(ImVec2(0.52f * window_size_width, 0.04f * window_size_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(0.44f * window_size_width, 0.92f * window_size_height), ImGuiCond_Always);
	flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::PushStyleColor(ImGuiCol_Text, window_color_font_title);
	ImGui::Begin("Connection Information", NULL, flags);
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_Text, window_color_font_inside);

	ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
	for (unsigned i = 0; i < myConnectInfo.size(); i++)
	{
		ImGui::Text(myConnectInfo[i].c_str());
		ImGui::SetScrollHereY();
	}
	ImGui::PopTextWrapPos();

	ImGui::PopStyleColor();
	ImGui::End();

	ImGui::PopStyleColor();
}

void UIManager::pushMessage(std::string message)
{
	myConnectInfo.push_back(message);
	while (myConnectInfo.size() > connect_info_max_len)
	{
		myConnectInfo.pop_front();
	}
}

void UIManager::popMessage()
{
	if (myConnectInfo.size() > 0)
	{
		myConnectInfo.pop_back();
	}
}

void UIManager::fps_control(Uint32& prev, Uint32& now)
{
	now = SDL_GetTicks();
	Uint32 delta = now - prev;
	Uint32 spf = static_cast<Uint32>(1000 / window_fps);
	if (delta < spf)
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(spf - delta)));
	prev = SDL_GetTicks();
}

bool UIManager::initialize_sdl()
{
	// init SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		showWindowsMessageError(std::string("Error: SDL_Init failed\n") + SDL_GetError());
		return false;
	}
	// setup GL context
	// GL 3.0 + GLSL 130
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	myWindow = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_size_width, window_size_height, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
	if (!myWindow)
	{
		showWindowsMessageError(std::string("Error: SDL_CreateWindow failed\n") + SDL_GetError());
		return false;
	}
	myGLContext = SDL_GL_CreateContext(myWindow);
	if (!myGLContext)
	{
		showWindowsMessageError(std::string("Error: SDL_GL_CreateContext failed\n") + SDL_GetError());
		return false;
	}
	SDL_GL_MakeCurrent(myWindow, myGLContext);
	SDL_GL_SetSwapInterval(1);
	return true;
}

bool UIManager::initialize_glew()
{
	// init glew
	if (glewInit() != GLEW_OK)
	{
		showWindowsMessageError("Error: glewInit failed\n");
		return false;
	}
	return true;
}

void UIManager::initialize_imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL; // disable ini file
	ImGui::StyleColorsClassic();
	ImGui_ImplSDL2_InitForOpenGL(myWindow, myGLContext);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void UIManager::quit_sdl()
{
	SDL_GL_DeleteContext(myGLContext);
	SDL_DestroyWindow(myWindow);
	SDL_Quit();
}

void UIManager::quit_imgui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void UIManager::showWindowsMessageError(const std::string message)
{
	// This function is only used for displaying error message that would cause the program terminate
	MessageBoxA(NULL, message.c_str(), "Android Trackpad Service Error", MB_OK);
}