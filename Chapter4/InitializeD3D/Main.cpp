#include <Windows.h>
#include <crtdbg.h>
#include "WindowsUtility.h"
#include "Application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE preInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Application application(hInstance);
		if (!application.Initialize())
		{
			return 0;
		}

		return application.Run();
	}
	catch(DxException e)
	{
		MessageBox(NULL, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}