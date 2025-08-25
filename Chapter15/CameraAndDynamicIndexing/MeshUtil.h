#include "D3DUtil.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include <map>

class MeshUtil
{
public:
	static unique_ptr<MeshGeometry> CreateMesh(
		string name,
		map<string, GeometryGenerator::MeshData> meshs,
		ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* cmdList)
	{
		UINT vertexOffset = 0;
		UINT indexOffset = 0;
		UINT totalVertexCount = 0;

		map<string, SubmeshGeometry> submeshs;

		for (auto& meshPair : meshs)
		{
			auto& mesh = meshPair.second;
			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)mesh.Indices32.size();
			submesh.StartIndexLocation = indexOffset;
			submesh.BaseVertexLocation = vertexOffset;

			submeshs[meshPair.first] = submesh;

			indexOffset += (UINT)mesh.Indices32.size();
			vertexOffset += (UINT)mesh.Vertices.size();
		}

		vector<Vertex> vertices(vertexOffset);
		vector<uint16_t> indices;
		UINT index = 0;

		for (auto& meshPair : meshs)
		{
			auto& mesh = meshPair.second;
			for (size_t i = 0; i < mesh.Vertices.size(); ++i, ++index)
			{
				vertices[index].Pos = mesh.Vertices[i].Position;
				vertices[index].Normal = mesh.Vertices[i].Normal;
				vertices[index].TexC = mesh.Vertices[i].TexC;
			}
			indices.insert(indices.end(), begin(mesh.GetIndices16()), end(mesh.GetIndices16()));
		}

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

		auto geo = make_unique<MeshGeometry>();
		geo->Name = name;

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(d3dDevice,
			cmdList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(d3dDevice,
			cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		for (auto& submesh : submeshs)
		{
			geo->DrawArgs[submesh.first] = submesh.second;
		}

		return geo;
	}
};
