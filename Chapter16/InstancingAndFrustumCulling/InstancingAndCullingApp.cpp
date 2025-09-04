#include "BaseApp.h"
#include "TextureUtil.h"
#include "StaticSamplers.h"
#include "MeshUtil.h"
#include "MaterialUtil.h"
#include "PSOUtil.h"

class InstancingAndCullingApp : public BaseApp
{
public:
	InstancingAndCullingApp(HINSTANCE hInstance);
	InstancingAndCullingApp(const InstancingAndCullingApp&) = delete;
	InstancingAndCullingApp& operator=(const InstancingAndCullingApp&) = delete;
	~InstancingAndCullingApp();

protected:
	virtual void Build() override;

private:

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildSkullGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSOs();
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int shwoCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InstancingAndCullingApp theApp(hInstance);
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

InstancingAndCullingApp::InstancingAndCullingApp(HINSTANCE hInstance)
	: BaseApp(hInstance)
{
}

InstancingAndCullingApp::~InstancingAndCullingApp()
{
	if (md3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

void InstancingAndCullingApp::Build()
{
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildSkullGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
}

void InstancingAndCullingApp::LoadTextures()
{
	auto bricksTex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "bricks");
	auto stoneTex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "stone");
	auto tileTex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "tile");
	auto crateTex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "WoodCrate01");
	auto iceTex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "ice");
	auto grassTex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "grass");
	auto defaultTex = TextureUtil::LoadTexture(md3dDevice.Get(), mCommandList.Get(), "white1x1");

	mTextures[bricksTex->Name] = move(bricksTex);
	mTextures[stoneTex->Name] = move(stoneTex);
	mTextures[tileTex->Name] = move(tileTex);
	mTextures[crateTex->Name] = move(crateTex);
	mTextures[iceTex->Name] = move(iceTex);
	mTextures[grassTex->Name] = move(grassTex);
	mTextures[defaultTex->Name] = move(defaultTex);
}

void InstancingAndCullingApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[objRootParameterIndex].InitAsShaderResourceView(0, 1);
	slotRootParameter[matBufferRootParameterIndex].InitAsShaderResourceView(1, 1);
	slotRootParameter[passCBRootParameterIndex].InitAsConstantBufferView(0);
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

void InstancingAndCullingApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = (UINT)mTextures.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	for (auto& tex : mTextures)
	{
		auto resource = tex.second->Resource;
		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	}
}

void InstancingAndCullingApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mStdInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void InstancingAndCullingApp::BuildSkullGeometry()
{
	auto skullGeo = MeshUtil::LoadMesh(md3dDevice.Get(), mCommandList.Get(), "skull");
	mGeometries["skullGeo"] = move(skullGeo);
}

void InstancingAndCullingApp::BuildMaterials()
{
	auto bricks0 = MaterialUtil::LoadMaterial(0, 0, "bricks0");
	auto stone0 = MaterialUtil::LoadMaterial(1, 1, "stone0");
	auto tile0 = MaterialUtil::LoadMaterial(2, 2, "tile0");
	auto crate0 = MaterialUtil::LoadMaterial(3, 3, "checkboard0");
	auto ice0 = MaterialUtil::LoadMaterial(4, 4, "ice0");
	auto grass0 = MaterialUtil::LoadMaterial(5, 5, "grass0");
	auto skullMat = MaterialUtil::LoadMaterial(6, 6, "skullMat");

	mMaterials[bricks0->Name] = move(bricks0);
	mMaterials[stone0->Name] = move(stone0);
	mMaterials[tile0->Name] = move(tile0);
	mMaterials[crate0->Name] = move(crate0);
	mMaterials[ice0->Name] = move(ice0);
	mMaterials[grass0->Name] = move(grass0);
	mMaterials[skullMat->Name] = move(skullMat);
}

void InstancingAndCullingApp::BuildRenderItems()
{
	auto skullRitem = make_unique<RenderItem>();
	skullRitem->World = MathHelper::Identity4x4();
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 0;
	skullRitem->Mat = mMaterials["skullMat"].get();
	skullRitem->Geo = mGeometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	skullRitem->Bounds = skullRitem->Geo->DrawArgs["skull"].Bounds;

	const int n = 5;
	auto instanceCount = n * n * n;
	skullRitem->Instances.resize(instanceCount);

	float width = 200.0f;
	float height = 200.0f;
	float depth = 200.0f;

	float x = -0.5f * width;
	float y = -0.5f * height;
	float z = -0.5f * depth;
	float dx = width / (n - 1);
	float dy = height / (n - 1);
	float dz = depth / (n - 1);

	for (int k = 0; k < n; ++k)
	{
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				int index = k * n * n + i * n + j;
				skullRitem->Instances[index].World = XMFLOAT4X4(
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					x + j * dx, y + i * dy, z + k * dz, 1.0f
				);

				XMStoreFloat4x4(&skullRitem->Instances[index].TexTransform, XMMatrixScaling(2.0f, 2.0f, 1.0f));
				skullRitem->Instances[index].MaterialIndex = index * mMaterials.size();
			}
		}
	}

	mAllRitems.push_back(move(skullRitem));

	for (auto& e : mAllRitems)
	{
		mRitemLayer[(int)RenderLayer::Opaque].push_back(e.get());
	}
}

void InstancingAndCullingApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void InstancingAndCullingApp::BuildPSOs()
{
	auto opaquePsoDesc = PSOUtil::MakeOpaquePSODesc(
		mShaders["standardVS"].Get(),
		mShaders["opaquePS"].Get(),
		mStdInputLayout,
		mRootSignature.Get(),
		mBackBufferFormat,
		mDepthStencilFormat,
		m4xMsaaState,
		m4xMsaaQuality);

	mPsoDescs["opaque"] = opaquePsoDesc;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&mPsoDescs["opaque"], IID_PPV_ARGS(&mPSOs["opaque"])));
}
