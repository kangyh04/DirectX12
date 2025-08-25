#include "BaseApp.h"
#include <iostream>

const int gNumFrameResources = 3;

BaseApp::BaseApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{

}

BaseApp::~BaseApp()
{

}

bool BaseApp::Initialize()
{
#if defined(DEBUG) | defined(_DEBUG)
	D3DApp::CreateDebugConsole();
	EnableD3D12DebugLayer();
#endif
	if (!D3DApp::Initialize())
	{
		return false;
	}

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return true;
}

void BaseApp::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void BaseApp::Update(const Timer& gt)
{
	OnKeyboardInput(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialBuffer(gt);
	UpdateMainPassCB(gt);
}

void BaseApp::Draw(const Timer& gt)
{
// 	string psoSuffix = mWireFrameMode ? "_wireframe" : "";
// 
// 	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
// 
// 	ThrowIfFailed(cmdListAlloc->Reset());
// 
// 	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["tessellation" + psoSuffix].Get()));
// 
// 	mCommandList->RSSetViewports(1, &mScreenViewport);
// 	mCommandList->RSSetScissorRects(1, &mScissorRect);
// 
// 	auto toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
// 		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
// 	mCommandList->ResourceBarrier(1, &toRenderTarget);
// 
// 	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
// 	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
// 
// 	auto currentBackBufferView = CurrentBackBufferView();
// 	auto depthStencilView = DepthStencilView();
// 	mCommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);
// 
// 	ID3D12DescriptorHeap* descriptorheaps[] = { mSrvDescriptorHeap.Get() };
// 	mCommandList->SetDescriptorHeaps(_countof(descriptorheaps), descriptorheaps);
// 
// 	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
// 
// 	auto passCB = mCurrFrameResource->PassCB->Resource();
// 	mCommandList->SetGraphicsRootConstantBufferView(passCBRootParameterIndex, passCB->GetGPUVirtualAddress());
// 
// 	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
// 	mCommandList->SetGraphicsRootShaderResourceView(matBufferRootParameterIndex, matBuffer->GetGPUVirtualAddress());
// 
// 	mCommandList->SetGraphicsRootDescriptorTable(texRootParameterIndex, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
// #pragma region RenderItems
// 
// 	mCommandList->SetPipelineState(mPSOs["opaque" + psoSuffix].Get());
// 	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
// 
// // 	mCommandList->SetPipelineState(mPSOs["alphaTested" + psoSuffix].Get());
// // 	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);
// // 
// // 	mCommandList->SetPipelineState(mPSOs["treeSprites" + psoSuffix].Get());
// // 	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);
// // 
// // 	mCommandList->SetPipelineState(mPSOs["transparent" + psoSuffix].Get());
// // 	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);
// #pragma endregion
// 
// 	auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
// 		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
// 	mCommandList->ResourceBarrier(1, &toPresent);
// 
// 	ThrowIfFailed(mCommandList->Close());
// 
// 	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
// 	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
// 
// 	ThrowIfFailed(mSwapChain->Present(0, 0));
// 	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
// 
// 	mCurrFrameResource->Fence = ++mCurrentFence;
// 
// 	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

		auto toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &toRenderTarget);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		auto currentBackBufferView = CurrentBackBufferView();
		auto depthStencilView = DepthStencilView();

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
	// set as a root descriptor.
	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	// Bind all the textures used in this scene.  Observe
	// that we only have to specify the first descriptor in the table.  
	// The root signature knows how many descriptors are expected in the table.
	mCommandList->SetGraphicsRootDescriptorTable(3, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

		auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &toPresent);

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void BaseApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void BaseApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BaseApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BaseApp::OnKeyboardInput(const Timer& gt)
{
	mWireFrameMode = GetAsyncKeyState('1') & 0x8000;

	const float dt = gt.GetDeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
	{
		mCamera.Walk(10.0f * dt);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		mCamera.Walk(-10.0f * dt);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		mCamera.Strafe(-10.0f * dt);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		mCamera.Strafe(10.0f * dt);
	}
	if (GetAsyncKeyState('Q') & 0x8000)
	{
		mCamera.Rise(-10.0f * dt);
	}
	if (GetAsyncKeyState('E') & 0x8000)
	{
		mCamera.Rise(10.0f * dt);
	}

	mCamera.UpdateViewMatrix();
}

void BaseApp::UpdateObjectCBs(const Timer& gt)
{
// 	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
// 
// 	for (auto& e : mAllRitems)
// 	{
// 		if (e->NumFramesDirty > 0)
// 		{
// 			XMMATRIX world = XMLoadFloat4x4(&e->World);
// 			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);
// 
// 			ObjectConstants objConstants;
// 			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
// 			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
// 			objConstants.MaterialIndex = e->Mat->MatCBIndex;
// 
// 			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
// 
// 			e->NumFramesDirty--;
// 		}
// 	}

	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			objConstants.MaterialIndex = e->Mat->MatCBIndex;

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void BaseApp::UpdateMaterialBuffer(const Timer& gt)
{
// 	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
// 	for (auto& e : mMaterials)
// 	{
// 		Material* mat = e.second.get();
// 		if (mat->NumFramesDirty > 0)
// 		{
// 			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
// 
// 			MaterialData matData;
// 			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
// 			matData.FresnelR0 = mat->FresnelR0;
// 			matData.Roughness = mat->Roughness;
// 			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
// 			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
// 
// 			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);
// 
// 			mat->NumFramesDirty--;
// 		}
// 	}

	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			cout << matData.DiffuseMapIndex << endl;

			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void BaseApp::UpdateMainPassCB(const Timer& gt)
{
// 	XMMATRIX view = mCamera.GetView();
// 	XMMATRIX proj = mCamera.GetProj();
// 
// 	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
// 	auto detView = XMMatrixDeterminant(view);
// 	auto detProj = XMMatrixDeterminant(proj);
// 	auto detViewProj = XMMatrixDeterminant(viewProj);
// 	XMMATRIX invView = XMMatrixInverse(&detView, view);
// 	XMMATRIX invProj = XMMatrixInverse(&detProj, proj);
// 	XMMATRIX invViewProj = XMMatrixInverse(&detViewProj, viewProj);
// 
// 	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
// 	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
// 	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
// 	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
// 	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
// 	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
// 
// 	mMainPassCB.EyePosW = mCamera.GetPosition3f();
// 	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
// 	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
// 	mMainPassCB.NearZ = 1.0f;
// 	mMainPassCB.FarZ = 1000.0f;
// 	mMainPassCB.TotalTime = gt.GetTotalTime();
// 	mMainPassCB.DeltaTime = gt.GetDeltaTime();
// 
// 	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
// 
// 	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
// 	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.8f };
// 	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
// 	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
// 	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
// 	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };
// 
// 	auto currPassCB = mCurrFrameResource->PassCB.get();
// 	currPassCB->CopyData(0, mMainPassCB);

	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	auto detView = XMMatrixDeterminant(view);
	auto detProj = XMMatrixDeterminant(proj);
	auto detViewProj = XMMatrixDeterminant(viewProj);
	XMMATRIX invView = XMMatrixInverse(&detView, view);
	XMMATRIX invProj = XMMatrixInverse(&detProj, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&detViewProj, viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.GetTotalTime();
	mMainPassCB.DeltaTime = gt.GetDeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void BaseApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const vector<RenderItem*>& ritems)
{
// 	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
// 
// 	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
// 
// 	for (size_t i = 0; i < ritems.size(); ++i)
// 	{
// 		auto ri = ritems[i];
// 
// 		auto vertexBufferView = ri->Geo->VertexBufferView();
// 		auto indexBufferView = ri->Geo->IndexBufferView();
// 		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
// 		cmdList->IASetIndexBuffer(&indexBufferView);
// 		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
// 
// 		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
// 
// 		cmdList->SetGraphicsRootConstantBufferView(objRootParameterIndex, objCBAddress);
// 
// 		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
// 	}

	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		auto vertexBufferView = ri->Geo->VertexBufferView();
		auto indexBufferView = ri->Geo->IndexBufferView();
		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		cmdList->IASetIndexBuffer(&indexBufferView);
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;

		// CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		// tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void BaseApp::BuildWireFramePSOs()
{
	for (auto& desc : mPsoDescs)
	{
		auto psoDesc = desc.second;
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		ComPtr<ID3D12PipelineState> wireframePSO;
		ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&wireframePSO)));
		mPSOs[desc.first + "_wireframe"] = wireframePSO;
	}
}

void BaseApp::EnableD3D12DebugLayer()
{
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();

		ComPtr<ID3D12Debug1> debugController1;
		if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
		{
			debugController1->SetEnableGPUBasedValidation(true);
		}
	}
}
