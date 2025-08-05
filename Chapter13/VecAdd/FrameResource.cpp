#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	PassCB = make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	// ObjectCB = make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
	// MaterialCB = make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
}

FrameResource::~FrameResource()
{
}