#pragma once
#include <Windows.h>
#include <string>

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString() const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)												\
{																		\
HRESULT hResult = (x);													\
std::wstring wfn = AnsiToWString(__FILE__);								\
if(FAILED(hResult)){throw DxException(hResult, L#x, wfn, __LINE__);}	\
}
#endif
