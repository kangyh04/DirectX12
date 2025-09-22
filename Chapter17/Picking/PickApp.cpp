#include "BaseApp.h"
#include "Input.h"
#include "TextureUtil.h"
#include "StaticSamplers.h"
#include "MeshUtil.h"
#include "MaterialUtil.h"
#include "PSOUtil.h"

class PickApp : public BaseApp
{
public:
	PickApp(HINSTANCE hInstance);
	PickApp(const PickApp&) = delete;
	PickApp& operator=(const PickApp&) = delete;
	~PickApp();

protected:
	virtual void Build() override;

private:
	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildCarGeometry();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildPSOs();

	void Pick(int sx, int sy);
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		PickApp theApp(hInstance);
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

PickApp::PickApp(HINSTANCE hInstance)
	:BaseApp(hInstance)
{

}

PickApp::~PickApp()
{
	if (md3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

void PickApp::Build()
{
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildCarGeometry();
	BuildFrameResources();
	BuildMaterials();
	BuildRenderItems();
	BuildPSOs();
}

void PickApp::LoadTextures()
{
	auto tex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "white1x1");
	mTextures[tex->Name] = move(tex);
}

void PickApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[objRootParameterIndex].InitAsConstantBufferView(0);
	slotRootParameter[passCBRootParameterIndex].InitAsConstantBufferView(1);
	slotRootParameter[matBufferRootParameterIndex].InitAsShaderResourceView(0, 1);
	slotRootParameter[texRootParameterIndex].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = StaticSampler::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
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
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void PickApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto tex = mTextures["white1x1"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = tex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(tex.Get(), &srvDesc, hDescriptor);
}

void PickApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = D3DUtil::CompileShader(L"shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mStdInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void PickApp::BuildCarGeometry()
{
	auto mesh = MeshUtil::LoadMesh(md3dDevice.Get(), mCommandList.Get(), "car");

	mGeometries[mesh->Name] = move(mesh);
}

void PickApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void PickApp::BuildMaterials()
{
	auto gray = MaterialUtil::LoadMaterial(0, 0, "gray0");
	auto highlight = MaterialUtil::LoadMaterial(1, 0, "highlight0");

	mMaterials[gray->Name] = move(gray);
	mMaterials[highlight->Name] = move(highlight);
}

void PickApp::BuildRenderItems()
{
	auto carRitem = make_unique<RenderItem>();
	XMStoreFloat4x4(&carRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	XMStoreFloat4x4(&carRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	carRitem->ObjCBIndex = 0;
	carRitem->Mat = mMaterials["gray0"].get();
	carRitem->Geo = mGeometries["car"].get();
	carRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	carRitem->Bounds = carRitem->Geo->DrawArgs["car"].Bounds;
	carRitem->IndexCount = carRitem->Geo->DrawArgs["car"].IndexCount;
	carRitem->StartIndexLocation = carRitem->Geo->DrawArgs["car"].StartIndexLocation;
	carRitem->BaseVertexLocation = carRitem->Geo->DrawArgs["car"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(carRitem.get());

	auto pickedRitem = make_unique<RenderItem>();
	pickedRitem->World = MathHelper::Identity4x4();
	pickedRitem->TexTransform = MathHelper::Identity4x4();
	pickedRitem->ObjCBIndex = 1;
	pickedRitem->Mat = mMaterials["highlight0"].get();
	pickedRitem->Geo = mGeometries["car"].get();
	pickedRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	pickedRitem->Visible = false;

	pickedRitem->IndexCount = 0;
	pickedRitem->StartIndexLocation = 0;
	pickedRitem->BaseVertexLocation = 0;
	mPickedRitem = pickedRitem.get();
	mRitemLayer[(int)RenderLayer::Highlight].push_back(pickedRitem.get());

	mAllRitems.push_back(move(carRitem));
	mAllRitems.push_back(move(pickedRitem));
}

void PickApp::BuildPSOs()
{
	auto opaquePSO = PSOUtil::MakeOpaquePSODesc(
		mShaders["standardVS"].Get(),
		mShaders["opaquePS"].Get(),
		mStdInputLayout,
		mRootSignature.Get(),
		mBackBufferFormat,
		mDepthStencilFormat,
		m4xMsaaState,
		m4xMsaaQuality);

	mPsoDescs["opaque"] = opaquePSO;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePSO, IID_PPV_ARGS(&mPSOs["opaque"])));

	auto highlightPsoDesc = opaquePSO;

	highlightPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	highlightPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;

	mPsoDescs["highlight"] = highlightPsoDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&highlightPsoDesc, IID_PPV_ARGS(&mPSOs["highlight"])));
}

void PickApp::Pick(int sx, int sy)
{
	XMFLOAT4X4 p = mCamera.GetProj4x4f();

	float vx = (2.0f * sx / mClientHeight - 1.0f) / p(0, 0);
	float vy = (-2.0f * sy / mClientHeight + 1.0f) / p(1, 1);

	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	XMMATRIX v = mCamera.GetView();
	XMVECTOR detView = XMMatrixDeterminant(v);
	XMMATRIX invView = XMMatrixInverse(&detView, v);

	mPickedRitem->Visible = false;

	for (auto ri : mRitemLayer[(int)RenderLayer::Opaque])
	{
		auto geo = ri->Geo;
		if (ri->Visible == false)
		{
			continue;
		}
	}
}
