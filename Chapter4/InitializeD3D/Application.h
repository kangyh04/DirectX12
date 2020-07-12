#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <Windows.h>
#include <windowsx.h>
#include <string>
#include "Timer.h"
#include "D3D12API.h"

class Application
{
public:
	Application(HINSTANCE hInstance);
	Application(const Application& other) = delete;
	Application& operator=(const Application& other) = delete;
	virtual ~Application() {}
public:
	static Application* GetApplication();

	int Run();
	bool Initialize();

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	int GetWidth();
	int GetHeight();
	HWND GetWindow();
protected:
	virtual void OnMouseDown(WPARAM buttonState, int x, int y) {}
	virtual void OnMouseUp(WPARAM buttonState, int x, int y) {}
	virtual void OnMouseMove(WPARAM buttonState, int x, int y) {}
private:
	void Update(const Timer& timer);
	void Draw(const Timer& timer);
	bool InitializeWindow();
	void CalculateFrameStats();
protected:
	static Application* application;
private:
	HINSTANCE appInstance = NULL;
	HWND window = NULL;
	std::wstring windowCaption = L"d3d App";

	Timer appTimer;
	D3D12API d3dAPI;

	int screenWidth = 800;
	int screenHeight = 600;
	bool appPaused = false;
	bool minimized = false;
	bool maximized = false;
	bool resizing = false;
};
