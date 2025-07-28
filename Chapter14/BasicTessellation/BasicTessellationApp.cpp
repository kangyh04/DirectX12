#include "BaseApp.h"
#include "StaticSamplers.h"

class BasicTessellationApp : public BaseApp
{
public:
	BasicTessellationApp(HINSTANCE hInstance);
	BasicTessellationApp(const BasicTessellationApp&) = delete;
	BasicTessellationApp& operator=(const BasicTessellationApp&) = delete;
	~BasicTessellationApp();

	virtual bool Initialize() override;
private:
	virtual void Update(const Timer& gt) override;
	virtual void OnKeyboardInput(const Timer& gt) override;
	virtual void AnimateMaterials(const Timer& gt) override;

	void LoadTextures();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildQuadPatchGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSOs();
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	try
	{
		BasicTessellationApp theApp(hInstance);
		if (!theApp.Initialize())
		{
			return 0;
		}
		return theApp.Run();
	}
	catch (const DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

BasicTessellationApp::BasicTessellationApp(HINSTANCE hInstance)
	: BaseApp(hInstance)
{
}

BasicTessellationApp::~BasicTessellationApp()
{
	if (md3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool BasicTessellationApp::Initialize()
{
	if (!BaseApp::Initialize())
	{
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	LoadTextures();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildQuadPatchGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void BasicTessellationApp::Update(const Timer& gt)
{
	BaseApp::Update(gt);
}

void BasicTessellationApp::OnKeyboardInput(const Timer& gt)
{

}

void BasicTessellationApp::AnimateMaterials(const Timer& gt)
{

}

void BasicTessellationApp::LoadTextures()
{

}

void BasicTessellationApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameters[3];

	slotRootParameters[objRootParameterIndex].InitAsConstantBufferView(0);
	slotRootParameters[passCBRootParameterIndex].InitAsConstantBufferView(1);
	slotRootParameters[matCBRootParameterIndex].InitAsConstantBufferView(2);

	auto staticSamplers = StaticSampler::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		3,
		slotRootParameters,
		(UINT)staticSamplers.size(),
		staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> error;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		error.GetAddressOf());

	if (error != nullptr)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void BasicTessellationApp::BuildShadersAndInputLayout()
{
	mShaders["tessVS"] = D3DUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["tessHS"] = D3DUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["tessDS"] = D3DUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["tessPS"] = D3DUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "PS", "ps_5_0");

	mStdInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void BasicTessellationApp::BuildQuadPatchGeometry()
{
	array<XMFLOAT3, 4> vertices =
	{
		XMFLOAT3(-10.0f, 0.0f,  10.0f),
		XMFLOAT3( 10.0f, 0.0f,  10.0f),
		XMFLOAT3(-10.0f, 0.0f, -10.0f),
		XMFLOAT3( 10.0f, 0.0f, -10.0f),
	};

	array<uint16_t, 4> indices = { 0, 1, 2, 3 };

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(XMFLOAT3);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = make_unique<MeshGeometry>();
	geo->Name = "quadpatchGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(XMFLOAT3);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["quadpatch"] = submesh;

	mGeometries[geo->Name] = move(geo);
}

void BasicTessellationApp::BuildMaterials()
{
	auto whiteMat = make_unique<Material>();
	whiteMat->Name = "whiteMat";
	whiteMat->MatCBIndex = 0;
	whiteMat->DiffuseSrvHeapIndex = 0;
	whiteMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	whiteMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	whiteMat->Roughness = 0.5f;

	mMaterials[whiteMat->Name] = move(whiteMat);
}

void BasicTessellationApp::BuildRenderItems()
{
	auto quadPatchRitem = make_unique<RenderItem>();
	quadPatchRitem->World = MathHelper::Identity4x4();
	quadPatchRitem->TexTransform = MathHelper::Identity4x4();
	quadPatchRitem->ObjCBIndex = 0;
	quadPatchRitem->Mat = mMaterials["whiteMat"].get();
	quadPatchRitem->Geo = mGeometries["quadpatchGeo"].get();
	quadPatchRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	quadPatchRitem->IndexCount = quadPatchRitem->Geo->DrawArgs["quadpatch"].IndexCount;
	quadPatchRitem->StartIndexLocation = quadPatchRitem->Geo->DrawArgs["quadpatch"].StartIndexLocation;
	quadPatchRitem->baseVertexLocation = quadPatchRitem->Geo->DrawArgs["quadpatch"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(quadPatchRitem.get());

	mAllRitems.push_back(move(quadPatchRitem));
}

void BasicTessellationApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void BasicTessellationApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessVS"]->GetBufferPointer()),
		mShaders["tessVS"]->GetBufferSize()
	};
	psoDesc.HS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessHS"]->GetBufferPointer()),
		mShaders["tessHS"]->GetBufferSize()
	};
	psoDesc.DS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessDS"]->GetBufferPointer()),
		mShaders["tessDS"]->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessPS"]->GetBufferPointer()),
		mShaders["tessPS"]->GetBufferSize()
	};

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(&mPSOs["tessellation"])));
}