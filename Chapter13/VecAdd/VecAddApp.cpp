#include "BaseApp.h"

struct Data
{
	XMFLOAT3 v1;
	XMFLOAT2 v2;
};

class VecAddApp : public BaseApp
{
public:
	VecAddApp(HINSTANCE hInstance);
	VecAddApp(VecAddApp& rhs) = delete;
	VecAddApp& operator=(VecAddApp& rhs) = delete;
	~VecAddApp() override;

	virtual bool Initialize() override;
private:
	virtual void Update(const Timer& gt) override;
	virtual void AnimateMaterials(const Timer& gt) override;

	void DoComputeWork();

	void BuildBuffers();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildPSOs();
	void BuildFrameResources();

private:
	const int NumDataElements = 32;

	ComPtr<ID3D12Resource> mInputBufferA, mInputUploadBufferA = nullptr;
	ComPtr<ID3D12Resource> mInputBufferB, mInputUploadBufferB = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	ComPtr<ID3D12Resource> mReadBackBuffer = nullptr;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		VecAddApp theApp(hInstance);
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

VecAddApp::VecAddApp(HINSTANCE hInstance)
	: BaseApp(hInstance)
{
}

VecAddApp::~VecAddApp()
{
	if (md3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool VecAddApp::Initialize()
{
	if (!BaseApp::Initialize())
	{
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildBuffers();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildPSOs();
	BuildFrameResources();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	DoComputeWork();

	return true;
}

void VecAddApp::Update(const Timer& gt)
{
}

void VecAddApp::AnimateMaterials(const Timer& gt)
{

}

void VecAddApp::DoComputeWork()
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSOs["vecAddCS"].Get()));

	mCommandList->SetComputeRootSignature(mRootSignature.Get());

	mCommandList->SetComputeRootShaderResourceView(0, mInputBufferA->GetGPUVirtualAddress());
	mCommandList->SetComputeRootShaderResourceView(1, mInputBufferB->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mOutputBuffer->GetGPUVirtualAddress());

	mCommandList->Dispatch(1, 1, 1);

	auto toCopy = CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

	mCommandList->ResourceBarrier(1, &toCopy);

	mCommandList->CopyResource(mReadBackBuffer.Get(), mOutputBuffer.Get());

	auto toReadBack = CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	Data* mappedData = nullptr;

	ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

	ofstream fout("results.txt");

	for (int i = 0; i < NumDataElements; ++i)
	{
		fout << "v1: (" << mappedData[i].v1.x << ", " << mappedData[i].v1.y << ", " << mappedData[i].v1.z << ") "
			<< "v2: (" << mappedData[i].v2.x << ", " << mappedData[i].v2.y << ")\n";
	}
}

void VecAddApp::BuildBuffers()
{
	vector<Data> dataA(NumDataElements);
	vector<Data> dataB(NumDataElements);

	for (int i = 0; i < NumDataElements; ++i)
	{
		dataA[i].v1 = XMFLOAT3(i, i, i);
		dataA[i].v2 = XMFLOAT2(i, 0);

		dataB[i].v1 = XMFLOAT3(-i, i, 0.0f);
		dataB[i].v2 = XMFLOAT2(0, -i);
	}

	UINT byteSize = dataA.size() * sizeof(Data);

	mInputBufferA = D3DUtil::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		dataA.data(),
		byteSize,
		mInputUploadBufferA);

	mInputBufferB = D3DUtil::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		dataB.data(),
		byteSize,
		mInputUploadBufferB);

	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto uavDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&uavDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mOutputBuffer)));

	auto readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	auto readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&readbackHeap,
		D3D12_HEAP_FLAG_NONE,
		&readbackDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
}

void VecAddApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameters[3];

	slotRootParameters[0].InitAsShaderResourceView(0);
	slotRootParameters[1].InitAsShaderResourceView(1);
	slotRootParameters[2].InitAsUnorderedAccessView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		3,
		slotRootParameters,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void VecAddApp::BuildDescriptorHeaps()
{

}

void VecAddApp::BuildShadersAndInputLayout()
{
	mShaders["vecAddCS"] = D3DUtil::CompileShader(L"Shaders\\VecAdd.hlsl", nullptr, "CS", "cs_5_0");
}

void VecAddApp::BuildPSOs()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};

	ZeroMemory(&computePsoDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));

	computePsoDesc.pRootSignature = mRootSignature.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(mShaders["vecAddCS"]->GetBufferPointer()),
		mShaders["vecAddCS"]->GetBufferSize()
	};
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	ThrowIfFailed(md3dDevice->CreateComputePipelineState(
		&computePsoDesc,
		IID_PPV_ARGS(&mPSOs["vecAddCS"])));
}

void VecAddApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(make_unique<FrameResource>(md3dDevice.Get(), 1, 0, 0));
	}
}