#pragma once

#include "D3D12Utility.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3D12API
{
public:
	bool InitializeAPI();
	bool GeneratedDevice();
	void Resize();

	void Set4XMsaaState(bool state);
	bool Get4XMsaaState() const;

private:
	bool InitializeDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();

	void FlushCommandQueue();

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 currentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> directCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	static const int SwapChainBufferCount = 2;
	int currentBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;

	D3D12_VIEWPORT screenViewPort;
	D3D12_RECT scissorRect;

	UINT rtvDescriptorSize = 0;
	UINT dsvDescriptorSize = 0;
	UINT cbvSrvUavDescriptorSize = 0;

	D3D_DRIVER_TYPE d2dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	bool msaaState = false;
	UINT msaaQuality = 0;
};
