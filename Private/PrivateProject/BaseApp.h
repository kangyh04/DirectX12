#pragma once

#include "D3DApp.h"
#include "FrameResource.h"
#include "RenderItem.h"

class BaseApp : public D3DApp
{
public:
	BaseApp(HINSTANCE hInstance);
	BaseApp(const BaseApp& rhs) = delete;
	BaseApp& operator=(const BaseApp& rhs) = delete;
	~BaseApp();
public:
	virtual bool Initialize() override;

	virtual void OnResize() override;
	virtual void Update(const Timer& gt) override;
	virtual void Draw(const Timer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	virtual void OnKeyboardInput(const Timer& gt);
	void UpdateCamera(const Timer& gt);

	virtual void AnimateMaterials(const Timer& gt) = 0;
	void UpdateObjectCBs(const Timer& gt);
	void UpdateMaterialCBs(const Timer& gt);
	void UpdateMainPassCB(const Timer& gt);

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const vector<RenderItem*>& ritems);

	void BuildWireFramePSOs();

	void EnableD3D12DebugLayer();

protected:
	bool mWireFrameMode = false;

	vector<unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	UINT objRootParameterIndex = 0;
	UINT passCBRootParameterIndex = 1;
	UINT matCBRootParameterIndex = 2;
	UINT texRootParameterIndex = 3;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	unordered_map<string, unique_ptr<MeshGeometry>> mGeometries;
	unordered_map<string, unique_ptr<Texture>> mTextures;
	unordered_map<string, unique_ptr<Material>> mMaterials;
	unordered_map<string, ComPtr<ID3DBlob>> mShaders;
	unordered_map<string, ComPtr<ID3D12PipelineState>> mPSOs;
	unordered_map<string, D3D12_GRAPHICS_PIPELINE_STATE_DESC> mPsoDescs;

	vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
	vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;
	vector<D3D12_INPUT_ELEMENT_DESC> mLandInputLayout;

	vector<unique_ptr<RenderItem>> mAllRitems;

	vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	vector<Texture*> mTextureLayer[(int)TextureLayer::Count];

	PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta = 1.5f * XM_PI;
	float mPhi = 0.2f * XM_PI;
	float mRadius = 15.0f;

	POINT mLastMousePos;
};

