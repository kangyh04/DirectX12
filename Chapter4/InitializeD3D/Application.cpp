#include "Application.h"

LRESULT CALLBACK
ApplicationProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return Application::GetApplication()->MsgProc(hwnd, msg, wParam, lParam);
}

Application* Application::application = NULL;
Application* Application::GetApplication()
{
	return application;
}

Application::Application(HINSTANCE hInstance)
	:appInstance(hInstance)
{
	assert(application == NULL);
	application = this;
}

int Application::Run()
{
	MSG msg = { 0 };

	appTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			appTimer.Tick();

			if (!appPaused)
			{
				CalculateFrameStats();
				Update(appTimer);
				Draw(appTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
}

bool Application::Initialize()
{
	if (!InitializeWindow())
	{
		return false;
	}
	if (!d3dAPI.InitializeAPI())
	{
		return false;
	}

	d3dAPI.Resize();

	return true;
}

LRESULT Application::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			appPaused = true;
			appTimer.Stop();
		}
		else
		{
			appPaused = false;
			appTimer.Start();
		}
		return 0;

	case WM_SIZE:
		screenWidth = LOWORD(lParam);
		screenHeight = HIWORD(lParam);

		if (d3dAPI.GeneratedDevice())
		{
			if (wParam == SIZE_MINIMIZED)
			{
				appPaused = true;
				minimized = true;
				maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				appPaused = false;
				minimized = false;
				maximized = true;

				d3dAPI.Resize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (minimized)
				{
					appPaused = false;
					minimized = false;
					d3dAPI.Resize();
				}
				else if (maximized)
				{
					appPaused = false;
					maximized = false;
					d3dAPI.Resize();
				}
				else if (resizing)
				{

				}
				else
				{
					d3dAPI.Resize();
				}
			}
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		appPaused = true;
		resizing = true;
		appTimer.Stop();
		return 0;

	case WM_EXITSIZEMOVE:
		appPaused = false;
		resizing = false;
		appTimer.Start();
		d3dAPI.Resize();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
		{
			d3dAPI.Set4XMsaaState(!d3dAPI.Get4XMsaaState());
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int Application::GetWidth()
{
	return screenWidth;
}

int Application::GetHeight()
{
	return screenHeight;
}

HWND Application::GetWindow()
{
	return window;
}

void Application::Update(const Timer& timer)
{

}

void Application::Draw(const Timer& timer)
{
	d3dAPI.Draw(timer);
}

bool Application::InitializeWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ApplicationProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = appInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return false;
	}

	RECT rect = { 0, 0, screenWidth, screenHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	window = CreateWindow(L"MainWnd", windowCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, appInstance, 0);
	if (!window)
	{
		MessageBox(0, L"CreateWindow Failed", 0, 0);
		return false;
	}

	ShowWindow(window, SW_SHOW);
	UpdateWindow(window);

	return true;
}

void Application::CalculateFrameStats()
{
	static int frameCount = 0;
	static float timeElapsed = 0.0f;

	++frameCount;

	if ((appTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCount;
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = windowCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(window, windowText.c_str());

		frameCount = 0;
		timeElapsed += 1.0f;
	}
}