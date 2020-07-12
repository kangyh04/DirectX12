#include "D3D12API.h"
#include "Application.h"
#include <DirectXColors.h>

using Microsoft::WRL::ComPtr;

bool D3D12API::InitializeAPI()
{
	if (!InitializeDirect3D())
	{
		return false;
	}

	Resize();

	return true;
}

bool D3D12API::GeneratedDevice()
{
	return d3dDevice;
}

void D3D12API::Resize()
{
	assert(d3dDevice);
	assert(swapChain);
	assert(directCmdListAlloc);

	FlushCommandQueue();

	ThrowIfFailed(commandList->Reset(directCmdListAlloc.Get(), NULL));

	for (int index = 0; index < SwapChainBufferCount; ++index)
	{
		swapChainBuffer[index].Reset();
	}
	depthStencilBuffer.Reset();

	ThrowIfFailed(swapChain->ResizeBuffers(
		SwapChainBufferCount,
		Application::GetApplication()->GetWidth(),
		Application::GetApplication()->GetHeight(),
		backBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	currentBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT index = 0; index < SwapChainBufferCount; ++index)
	{
		ThrowIfFailed(swapChain->GetBuffer(index, IID_PPV_ARGS(&swapChainBuffer[index])));
		d3dDevice->CreateRenderTargetView(swapChainBuffer[index].Get(), NULL, rtvHeapHandle);
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = Application::GetApplication()->GetWidth();
	depthStencilDesc.Height = Application::GetApplication()->GetHeight();
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	depthStencilDesc.SampleDesc.Count = msaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = msaaState ? (msaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = depthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = depthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	screenViewPort.TopLeftX = 0;
	screenViewPort.TopLeftY = 0;
	screenViewPort.Width = static_cast<float>(Application::GetApplication()->GetWidth());
	screenViewPort.Height = static_cast<float>(Application::GetApplication()->GetHeight());
	screenViewPort.MinDepth = 0.0f;
	screenViewPort.MaxDepth = 1.0f;

	scissorRect = { 0, 0, Application::GetApplication()->GetWidth(), Application::GetApplication()->GetHeight() };
}

void D3D12API::Set4XMsaaState(bool state)
{
	if (msaaState != state)
	{
		msaaState = state;

		CreateSwapChain();
		Resize();
	}
}

bool D3D12API::Get4XMsaaState() const
{
	return msaaState;
}

void D3D12API::Draw(const Timer& timer)
{
	ThrowIfFailed(directCmdListAlloc->Reset());

	ThrowIfFailed(commandList->Reset(directCmdListAlloc.Get(), NULL));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	commandList->RSSetViewports(1, &screenViewPort);
	commandList->RSSetScissorRects(1, &scissorRect);

	commandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, NULL);
	commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* cmds[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmds), cmds);

	auto reason = d3dDevice->GetDeviceRemovedReason();

	ThrowIfFailed(swapChain->Present(0, 0));
	currentBackBuffer = (currentBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

bool D3D12API::InitializeDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(
		NULL,
		D3D_FEATURE_LEVEL_12_1,
		IID_PPV_ARGS(&d3dDevice));

	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(&d3dDevice)));
	}

	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence)));

	rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbvSrvUavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = backBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	msaaQuality = msQualityLevels.NumQualityLevels;
	assert(msaaQuality > 0 && "Unexpected MSAA quality level");

#ifdef _DEBUG
	LogAdapters();
#endif
	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3D12API::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	ThrowIfFailed(d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(directCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		directCmdListAlloc.Get(),
		NULL,
		IID_PPV_ARGS(commandList.GetAddressOf())));

	commandList->Close();
}

void D3D12API::CreateSwapChain()
{
	swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc.Width = Application::GetApplication()->GetWidth();
	swapChainDesc.BufferDesc.Height = Application::GetApplication()->GetHeight();
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = backBufferFormat;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = msaaState ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = msaaState ? (msaaQuality - 1) : 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = SwapChainBufferCount;
	swapChainDesc.OutputWindow = Application::GetApplication()->GetWindow();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(dxgiFactory->CreateSwapChain(
		commandQueue.Get(),
		&swapChainDesc,
		swapChain.GetAddressOf()));
}

void D3D12API::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.GetAddressOf())));
}

void D3D12API::FlushCommandQueue()
{
	++currentFence;

	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFence));

	if (fence->GetCompletedValue() < currentFence)
	{
		// HANDLE eventHandle = CreateEventEx(NULL, false, false, EVENT_ALL_ACCESS);
		// HANDLE eventHandle = CreateEventEx(NULL, NULL, false, EVENT_ALL_ACCESS);
		HANDLE eventHandle = CreateEventExA(NULL, NULL, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(fence->SetEventOnCompletion(currentFence, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12API::DepthStencilView() const
{
	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* D3D12API::CurrentBackBuffer() const
{
	return swapChainBuffer[currentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12API::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		currentBackBuffer,
		rtvDescriptorSize);
}

void D3D12API::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = NULL;
	std::vector<IDXGIAdapter*> adapterList;
	while (dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter : ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);
		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3D12API::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = NULL;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***output : ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, backBufferFormat);

		ReleaseCom(output);

		++i;
	}
}
void D3D12API::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// for get list amount
	output->GetDisplayModeList(format, flags, &count, NULL);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;

		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		OutputDebugString(text.c_str());
	}
}
