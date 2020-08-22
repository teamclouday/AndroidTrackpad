#pragma once

#include <GL/glew.h>

#include <SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include <string>

// This class manage the UI
class UIManager
{
public:
	UIManager();
	~UIManager();
	void loop();
	static void showWindowsMessageError(const std::string message);
private:
	void draw_UI();
	void fps_control(Uint32& prev, Uint32& now);

	bool initialize_sdl();
	bool initialize_glew();
	void initialize_imgui();

	void quit_sdl();
	void quit_imgui();
public:
	const char* glsl_version = "#version 130";
	const char* window_title = "Android Trackpad Service";
	const int window_size_width = 600;
	const int window_size_height = 400;
	const int window_fps = 60;
	bool initialized = false;
private:
	SDL_Window* myWindow = nullptr;
	SDL_GLContext myGLContext = nullptr;
	const ImVec4 window_color_bg = ImVec4(0.2f, 0.5f, 1.0f, 1.0f);
	const ImVec4 window_color_cf = ImVec4(1.0f, 0.8f, 1.0f, 1.0f);
	const ImVec4 window_color_font_title = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	const ImVec4 window_color_font_inside = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	const ImVec4 window_color_radio_bt = ImVec4(0.4f, 0.4f, 1.0f, 1.0f);
	const ImVec4 window_color_bt = ImVec4(0.48, 0.73, 0.98f, 1.0f);
	const ImVec4 window_color_bt_hover = ImVec4(0.78, 0.98f, 1.0f, 1.0f);
};