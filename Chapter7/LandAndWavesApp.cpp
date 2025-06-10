#include "D3DApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"
#include "RenderItem.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;

class LandAndWavesApp : public D3DApp
{
public:
	LandAndWavesApp(HINSTANCE hInstance);
	LandAndWavesApp(const LandAndWavesApp& rhs) = delete;
	LandAndWavesApp& operator=(const LandAndWavesApp& rhs) = delete;
	~LandAndWavesApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const Timer& gt) override;
	virtual void Draw(const Timer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void OnKeyboardInput(const Timer& gt);
	void UpdateCamera(const Timer& gt);
	void UpdateObjectCBs(const Timer& gt);
	void UpdateMainPassCB(const Timer& gt);
	void UpdateWaves(const Timer& gt);

	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const vector<RenderItem*>& ritems);

	float GetHillsHeight(float x, float z) const;
	XMFLOAT3 GetHillsNormal(float x, float z) const;

private:
	vector<unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
	unordered_map<string, ComPtr<ID3DBlob>> mShaders;
	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;

	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	RenderItem* mWavesRitem = nullptr;

	vector<unique_ptr<RenderItem>> mAllRitems;

	vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	unique_ptr<Waves> mWaves;

	PassConstants mMainPassCB;

	bool mIsWireframe = false;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 5.0f;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	POINT mLastMousePos;
};

LandAndWavesApp::LandAndWavesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{

}

LandAndWavesApp::~LandAndWavesApp()
{
	if (md3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool LandAndWavesApp::Initialize()
{
#if defined(DEBUG) | defined(_DEBUG)
	D3DApp::CreateDebugConsole();
#endif

	if (!D3DApp::Initialize())
	{
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mWaves = make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildLandGeometry();
	BuildWavesGeometryBuffers();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	return true;
}

void LandAndWavesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, p);
}

void LandAndWavesApp::Update(const Timer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMainPassCB(gt);
	UpdateWaves(gt);
}

void LandAndWavesApp::Draw(const Timer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	if (mIsWireframe)
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	mCommandList->ResourceBarrier(1, &toRenderTarget);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	auto backBuffer = CurrentBackBufferView();
	auto depthStencilbuffer = DepthStencilView();
	mCommandList->OMSetRenderTargets(1, &backBuffer, true, &depthStencilbuffer);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	mCommandList->ResourceBarrier(1, &toPresent);

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrFrameResource->Fence = ++mCurrentFence;

	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void LandAndWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void LandAndWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void LandAndWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void LandAndWavesApp::OnKeyboardInput(const Timer& gt)
{
	if (GetAsyncKeyState('1') & 0x8000)
	{
		mIsWireframe = true;
	}
	else
	{
		mIsWireframe = false;
	}
}

void LandAndWavesApp::UpdateCamera(const Timer& gt)
{
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);

	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void LandAndWavesApp::UpdateObjectCBs(const Timer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			e->NumFramesDirty--;
		}
	}
}

void LandAndWavesApp::UpdateMainPassCB(const Timer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	auto viewDet = XMMatrixDeterminant(view);
	auto projDet = XMMatrixDeterminant(proj);
	auto viewProjDet = XMMatrixDeterminant(viewProj);

	XMMATRIX invView = XMMatrixInverse(&viewDet, view);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.GetTotalTime();
	mMainPassCB.DeltaTime = gt.GetDeltaTime();

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void LandAndWavesApp::UpdateWaves(const Timer& gt)
{
	static float t_base = 0.0f;
	if ((mTimer.GetTotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	mWaves->Update(gt.GetDeltaTime());

	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Color = XMFLOAT4(Colors::Blue);

		currWavesVB->CopyData(i, v);
	}

	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void LandAndWavesApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())
	));
}

void LandAndWavesApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = D3DUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["OpaquePS"] = D3DUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
	};
}

void LandAndWavesApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);

		if (vertices[i].Pos.y < -10.0f)
		{
			vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if (vertices[i].Pos.y < 5.0f)
		{
			vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if (vertices[i].Pos.y < 12.0f)
		{
			vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (vertices[i].Pos.y < 20.0f)
		{
			vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		
		vector<uint16_t> indices = grid.GetIndices16();
		const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

		auto geo = make_unique<MeshGeometry>();
		geo->Name = "landGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["grid"] = submesh;

		mGeometries["landGeo"] = move(geo);
	}
}

void LandAndWavesApp::BuildWavesGeometryBuffers()
{
	vector<uint16_t> indices(3 * mWaves->TriangleCount());
	assert(mWaves->VertexCount() < 0x0000ffff);

	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6;
		}
	}

	UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);
	
	auto geo = make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = move(geo);
}

void LandAndWavesApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;

	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	desc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	desc.pRootSignature = mRootSignature.Get();
	desc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	desc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = mBackBufferFormat;
	desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	desc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	desc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframeDesc = desc;
	wireframeDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wireframeDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
}

void LandAndWavesApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), mWaves->VertexCount()));
	}
}

void LandAndWavesApp::BuildRenderItems()
{
	auto wavesRitem = make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCBIndex = 0;
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->baseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Opaque].push_back(wavesRitem.get());

	auto gridRitem = make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->baseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	mAllRitems.push_back(move(wavesRitem));
	mAllRitems.push_back(move(gridRitem));
}

void LandAndWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();

	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		auto vertexBuffer = ri->Geo->VertexBufferView();
		auto indexBuffer = ri->Geo->IndexBufferView();
		cmdList->IASetVertexBuffers(0, 1, &vertexBuffer);
		cmdList->IASetIndexBuffer(&indexBuffer);
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += ri->ObjCBIndex * objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->baseVertexLocation, 0);
	}
}

float LandAndWavesApp::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 LandAndWavesApp::GetHillsNormal(float x, float z) const
{
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);
	return n;
}
