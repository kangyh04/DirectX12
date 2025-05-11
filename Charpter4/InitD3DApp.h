#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <wtypes.h>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class InitD3DApp
{
protected:
	InitD3DApp(HINSTANCE hInstance);
	InitD3DApp(const InitD3DApp& rhs) = delete;
	InitD3DApp& operator=(const InitD3DApp& rhs) = delete;
	virtual ~InitD3DApp();
};
