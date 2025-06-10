#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT wavesVertCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	PassCB = make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	ObjectCB = make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);

	WavesVB = make_unique<UploadBuffer<Vertex>>(device, wavesVertCount, false);
}

FrameResource::~FrameResource()
{
}