#pragma once

#include "D3DApp.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "Camera.h"
#include "FrustumCulling.h"
#include "CubeRenderTarget.h"
#include "ShadowMap.h"

const UINT CubeMapSize = 512;

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

	virtual void OnKeyInputed(LPARAM lParam) override;

	virtual void OnKeyboardInput(const Timer& gt);

	virtual void AnimateMaterials(const Timer& gt) {}
	void UpdateInstanceBuffer(const Timer& gt);
	void UpdateMaterialBuffer(const Timer& gt);
	void UpdateShadowTransform(const Timer& gt);
	void UpdateMainPassCB(const Timer& gt);
	void UpdateShadowPassCB(const Timer& gt);

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const vector<RenderItem*>& ritems);

	void DrawSceneToShadowMap();

	void BuildWireFramePSOs();

	void EnableD3D12DebugLayer();

protected:
	virtual void Build() {}

protected:
	bool mWireFrameMode = false;

	vector<unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	UINT objRootParameterIndex = 0;
	UINT matBufferRootParameterIndex = 1;
	UINT passCBRootParameterIndex = 2;
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

	vector<unique_ptr<RenderItem>> mAllRitems;

	vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	vector<Texture*> mTextureLayer[(int)TextureLayer::Count];

	PassConstants mMainPassCB;
	PassConstants mShadowPassCB;

	UINT mSkyTexHeapIndex = 0;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

	Camera mCamera;

	FrustumCulling mFrustumCulling;

	POINT mLastMousePos;

	Camera mCubeMapCameras[6];
	unique_ptr<ShadowMap> mShadowMap;
	BoundingSphere mSceneBounds;

	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] =
	{
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 mRotatedLightDirections[3];
};

