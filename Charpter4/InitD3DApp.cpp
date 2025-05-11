#include "D3DApp.h"
#include <DirectXColors.h>

using Microsoft::WRL::ComPtr;
using namespace std;

class InitD3DApp : public D3DApp
{
public:
	InitD3DApp(HINSTANCE hInstance);
	~InitD3DApp();
	virtual bool Initialize();

private:
	void Residency();
	void CheckFeature();

	virtual void OnResize() override;
	virtual void Update(const Timer& gt) override;
	virtual void Draw(const Timer& gt) override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdList, int showCmd)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InitD3DApp theApp(hInstance);

		if (!theApp.Initialize())
		{
			return 0;
		}
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

InitD3DApp::InitD3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

InitD3DApp::~InitD3DApp()
{
}

bool InitD3DApp::Initialize()
{
	if (!D3DApp::Initialize())
	{
		return false;
	}
	return true;
}

void InitD3DApp::Residency()
{
	auto num = 2;
	ID3D12Pageable* objects[2]
	{
	};

	// md3dDevice->MakeResident(num, objects);

	// md3dDevice->Evict(num, objects);
}

void InitD3DApp::CheckFeature()
{
	D3D_FEATURE_LEVEL featureLevels[3] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0
	};

D3D12_FEATURE_DATA_FEATURE_LEVELS featureInfo;
	featureInfo.NumFeatureLevels = 3;
	featureInfo.pFeatureLevelsRequested = featureLevels;

	// md3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureInfo, sizeof(featureInfo));
}

void InitD3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitD3DApp::Update(const Timer& gt)
{

}

void InitD3DApp::Draw(const Timer& gt)
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
// 	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
// 		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	mCommandList->ResourceBarrier(1, &barrier);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	auto backBufferView = CurrentBackBufferView();
	auto depthStencilView = DepthStencilView();
	// mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	mCommandList->OMSetRenderTargets(1, &backBufferView, true, &depthStencilView);

	CD3DX12_RESOURCE_BARRIER toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
// 	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
// 		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	mCommandList->ResourceBarrier(1, &toPresentBarrier);

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}
