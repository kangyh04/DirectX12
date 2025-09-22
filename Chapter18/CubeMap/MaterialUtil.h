#pragma once
#include "D3DUtil.h"

class MaterialUtil
{
public:
	static unique_ptr<Material> LoadMaterial(
		int matCBIndex,
		int diffuseSrvHeapIndex,
		string name)
	{
		ifstream fin("Materials/" + name + ".txt");

		if (!fin)
		{
			wstring msg = L"Materials/" + AnsiToWString(name) + L".txt not found.";
			MessageBox(0, msg.c_str(), 0, 0);
			return nullptr;
		}

		auto mat = make_unique<Material>();
		mat->Name = name;
		mat->MatCBIndex = matCBIndex;
		mat->DiffuseSrvHeapIndex = diffuseSrvHeapIndex;

		string ignore;

		fin >> ignore;
		fin >> mat->DiffuseAlbedo.x >> mat->DiffuseAlbedo.y >> mat->DiffuseAlbedo.z >> mat->DiffuseAlbedo.w;
		fin >> mat->FresnelR0.x >> mat->FresnelR0.y >> mat->FresnelR0.z;
		fin >> mat->Roughness;

		fin.close();
		return mat;
	}
};
