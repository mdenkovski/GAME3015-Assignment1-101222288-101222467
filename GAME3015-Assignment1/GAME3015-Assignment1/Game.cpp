//***************************************************************************************
// Game.cpp 
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "../../Common/Camera.h"
#include "FrameResource.h"
#include "Waves.h"
#include <ctime>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class Game : public D3DApp
{
public:
	Game(HINSTANCE hInstance);
	Game(const Game& rhs) = delete;
	Game& operator=(const Game& rhs) = delete;
	~Game();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	//void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayouts();

	void BuildLandGeometry();

	void BuildCityLandGeometry();
	void BuildGroundGeometry();
	void BuildCNTowerGeometry();
	void BuildRogersCenter();
	void BuildBuildings();
	void BuildWavesGeometry();
	void BuildBoxGeometry();
	void BuildDiamondGeometry();
	void BuildTreeSpritesGeometry();


	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderPyra(float x, float y, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex);
	void BuildRenderFlatTopPyra(float x, float y, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex);
	void BuildRenderBuilding(float x, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex);
	void BuildRenderRoad(float x, float z, float length,float width, float angle, UINT& ObjCBIndex);
	void BuildRenderIntersection(float x, float z, float scalex, float scalez, UINT& ObjCBIndex);
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

	RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	std::vector<BoundingBox> mBoundingBoxes;
	std::vector<BoundingSphere> mBoundingSpheres;

	std::unique_ptr<Waves> mWaves;

	PassConstants mMainPassCB;

	/*XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 20.0f;*/


	Camera mCamera;

	POINT mLastMousePos;

	float scaleFactor = 10.0f;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Game theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException & e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

Game::Game(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

Game::~Game()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool Game::Initialize()
{
	srand((unsigned)time(0));
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mCamera.SetPosition(0.8f * scaleFactor, 0.3 * scaleFactor, 1.0f * scaleFactor);

	mWaves = std::make_unique<Waves>(40, 80, 0.25, 0.03f, 1.0, 0.2f);

	//Step 1 Load the textures
	LoadTextures();

	BuildRootSignature();

	//Step 2 Build Descriptor heaps to allow for the new texture
	BuildDescriptorHeaps();
	BuildShadersAndInputLayouts();

	// Step 3 Build the geometry for your shapes
	//BuildLandGeometry();
	BuildCityLandGeometry();
	BuildGroundGeometry();
	BuildCNTowerGeometry();
	BuildRogersCenter();
	BuildBuildings();
	BuildWavesGeometry();
	//BuildBoxGeometry();
	BuildDiamondGeometry();
	BuildTreeSpritesGeometry();

	// Step 4 Build the material
	BuildMaterials();

	// Step 5 Build your render items
	BuildRenderItems();

	BuildFrameResources();
	BuildPSOs();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void Game::OnResize()
{
	D3DApp::OnResize();

	//// The window resized, so update the aspect ratio and recompute the projection matrix.
	//XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	//XMStoreFloat4x4(&mProj, P);

	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void Game::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	//UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this roof point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateWaves(gt);
}

void Game::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the roof value to mark commands up to this roof point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new roof point. 
	// Because we are on the GPU timeline, the new roof point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void Game::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Game::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Game::OnMouseMove(WPARAM btnState, int x, int y)
{
	//if ((btnState & MK_LBUTTON) != 0)
	//{
	//	// Make each pixel correspond to a quarter of a degree.
	//	float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
	//	float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

	//	// Update angles based on input to orbit camera around box.
	//	mTheta += dx;
	//	mPhi += dy;

	//	// Restrict the angle mPhi.
	//	mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	//}
	//else if ((btnState & MK_RBUTTON) != 0)
	//{
	//	// Make each pixel correspond to 0.2 unit in the scene.
	//	float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
	//	float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

	//	// Update the camera radius based on input.
	//	mRadius += dx - dy;

	//	// Restrict the radius.
	//	mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	//}

	//mLastMousePos.x = x;
	//mLastMousePos.y = y;

	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Game::OnKeyboardInput(const GameTimer& gt)
{

	const float dt = gt.DeltaTime();

	mCamera.GetLook();
	float tmin = 0;
	float buffer = 0.5 * scaleFactor;
	XMFLOAT3  oppositef3(-1, -1, -1);
	XMVECTOR opposite = XMLoadFloat3(&oppositef3);

	if (GetAsyncKeyState('W') & 0x8000)
	{
		bool hit = false;
		for (auto box : mBoundingBoxes)
		{
			if (box.Intersects(mCamera.GetPosition(), mCamera.GetLook(), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		for (auto sphere : mBoundingSpheres)
		{
			if (sphere.Intersects(mCamera.GetPosition(), mCamera.GetLook(), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		if (!hit)
		{
				mCamera.Walk(10.0f*dt);
			
		}
		
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		bool hit = false;
		for (auto box : mBoundingBoxes)
		{
			if (box.Intersects(mCamera.GetPosition(), XMVectorMultiply( mCamera.GetLook(), opposite), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		for (auto sphere : mBoundingSpheres)
		{
			if (sphere.Intersects(mCamera.GetPosition(), mCamera.GetLook(), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		if (!hit)
		{
			mCamera.Walk(-10.0f*dt);
		}
		
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		bool hit = false;
		for (auto box : mBoundingBoxes)
		{
			if (box.Intersects(mCamera.GetPosition(), XMVectorMultiply(mCamera.GetRight(), opposite), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		for (auto sphere : mBoundingSpheres)
		{
			if (sphere.Intersects(mCamera.GetPosition(), mCamera.GetLook(), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		if (!hit)
		{
			mCamera.Strafe(-10.0f*dt);
		}
		
		
	}
		

	if (GetAsyncKeyState('D') & 0x8000)
	{
		bool hit = false;
		for (auto box : mBoundingBoxes)
		{
			if (box.Intersects(mCamera.GetPosition(), mCamera.GetRight(), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		for (auto sphere : mBoundingSpheres)
		{
			if (sphere.Intersects(mCamera.GetPosition(), mCamera.GetLook(), tmin))
			{
				if (tmin < buffer)
				{
					hit = true;
				}
			}
		}
		if (!hit)
		{
			mCamera.Strafe(10.0f*dt);
		}
	}
		

	mCamera.UpdateViewMatrix();
	mCamera.SetPositionY(0.3 * scaleFactor);
}
//
//void Game::UpdateCamera(const GameTimer& gt)
//{
//	// Convert Spherical to Cartesian coordinates.
//	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
//	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
//	mEyePos.y = mRadius * cosf(mPhi);
//
//	// Build the view matrix.
//	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
//	XMVECTOR target = XMVectorZero();
//	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//
//	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
//	XMStoreFloat4x4(&mView, view);
//}

void Game::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void Game::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void Game::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void Game::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };
	mMainPassCB.Lights[3].Position = { -5.0f  * scaleFactor, 3.0f * scaleFactor, 0.0f * scaleFactor };
	mMainPassCB.Lights[3].Strength = { 1.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[3].Direction = { 0.0, -1.0, 0.0f };
	mMainPassCB.Lights[3].FalloffStart = { 1.0f  * scaleFactor };
	mMainPassCB.Lights[3].FalloffEnd = { 5.0f  * scaleFactor };
	mMainPassCB.Lights[3].SpotPower = { 1.0f };


	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void Game::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);

		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void Game::LoadTextures()
{
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../../Textures/grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));
	mTextures[grassTex->Name] = std::move(grassTex);

	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../../Textures/water1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));
	mTextures[waterTex->Name] = std::move(waterTex);

	auto roofTex = std::make_unique<Texture>();
	roofTex->Name = "roofTex";
	roofTex->Filename = L"../../Textures/Roof.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), roofTex->Filename.c_str(),
		roofTex->Resource, roofTex->UploadHeap));
	mTextures[roofTex->Name] = std::move(roofTex);

	auto buildingTex = std::make_unique<Texture>();
	buildingTex->Name = "buildingTex";
	buildingTex->Filename = L"../../Textures/building.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), buildingTex->Filename.c_str(),
		buildingTex->Resource, buildingTex->UploadHeap));
	mTextures[buildingTex->Name] = std::move(buildingTex);

	auto concreteTex = std::make_unique<Texture>();
	concreteTex->Name = "concreteTex";
	concreteTex->Filename = L"../../Textures/concrete.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), concreteTex->Filename.c_str(),
		concreteTex->Resource, concreteTex->UploadHeap));
	mTextures[concreteTex->Name] = std::move(concreteTex);

	auto brickTex = std::make_unique<Texture>();
	brickTex->Name = "brickTex";
	brickTex->Filename = L"../../Textures/bricks.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), brickTex->Filename.c_str(),
		brickTex->Resource, brickTex->UploadHeap));
	mTextures[brickTex->Name] = std::move(brickTex);

	auto fenceTex = std::make_unique<Texture>();
	fenceTex->Name = "fenceTex";
	fenceTex->Filename = L"../../Textures/WireFence.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), fenceTex->Filename.c_str(),
		fenceTex->Resource, fenceTex->UploadHeap));
	mTextures[fenceTex->Name] = std::move(fenceTex);

	auto roadTex = std::make_unique<Texture>();
	roadTex->Name = "roadTex";
	roadTex->Filename = L"../../Textures/road.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), roadTex->Filename.c_str(),
		roadTex->Resource, roadTex->UploadHeap));
	mTextures[roadTex->Name] = std::move(roadTex);

	auto roadITex = std::make_unique<Texture>();
	roadITex->Name = "roadITex";
	roadITex->Filename = L"../../Textures/intersection.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), roadITex->Filename.c_str(),
		roadITex->Resource, roadITex->UploadHeap));
	mTextures[roadITex->Name] = std::move(roadITex);

	auto treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"../../Textures/treeArray.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource, treeArrayTex->UploadHeap));
	mTextures[treeArrayTex->Name] = std::move(treeArrayTex);


}

void Game::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void Game::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 10;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto grassTex = mTextures["grassTex"]->Resource;
	auto waterTex = mTextures["waterTex"]->Resource;
	auto roofTex = mTextures["roofTex"]->Resource;
	auto buildingTex = mTextures["buildingTex"]->Resource;
	auto concreteTex = mTextures["concreteTex"]->Resource;
	auto brickTex = mTextures["brickTex"]->Resource;
	auto fenceTex = mTextures["fenceTex"]->Resource;
	auto roadTex = mTextures["roadTex"]->Resource;
	auto roadITex = mTextures["roadITex"]->Resource;
	auto treeArrayTex = mTextures["treeArrayTex"]->Resource;


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = waterTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = roofTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(roofTex.Get(), &srvDesc, hDescriptor);

	// next descriptor - building texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = buildingTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(buildingTex.Get(), &srvDesc, hDescriptor);

	// next descriptor - concrete texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = concreteTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(concreteTex.Get(), &srvDesc, hDescriptor);

	// next descriptor - brick texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = brickTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(brickTex.Get(), &srvDesc, hDescriptor);

	// next descriptor - fence texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = fenceTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);

	// next descriptor - road texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = roadTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(roadTex.Get(), &srvDesc, hDescriptor);

	// next descriptor - roadI texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = roadITex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(roadITex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);


	auto desc = treeArrayTex->GetDesc();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);


}

void Game::BuildShadersAndInputLayouts()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mStdInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void Game::BuildLandGeometry()
{

	GeometryGenerator geoGen;

	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	//mGeometries["landGeo"] = std::move(geo);

	mGeometries[geo->Name] = std::move(geo);
}

void Game::BuildCityLandGeometry()
{

	GeometryGenerator geoGen;

	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	//mGeometries["landGeo"] = std::move(geo);

	mGeometries[geo->Name] = std::move(geo);
}

void Game::BuildGroundGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData ground = geoGen.CreateBox(20, 0.2, 20, 1);


	std::vector<Vertex> vertices(ground.Vertices.size());
	for (size_t i = 0; i < ground.Vertices.size(); ++i)
	{
		auto& p = ground.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = ground.Vertices[i].Normal;
		vertices[i].TexC = ground.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = ground.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "groundGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	

	geo->DrawArgs["ground"] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void Game::BuildCNTowerGeometry()
{
	GeometryGenerator geoGen;

	GeometryGenerator::MeshData towerCylinder = geoGen.CreateCylinder(0.5, 0.4f, 1.0f, 20, 16);
	GeometryGenerator::MeshData towerWedge = geoGen.CreateWedge(1.0f, 1.0, 1.0f, 0);
	GeometryGenerator::MeshData towerPlatformSphere = geoGen.CreateSphere(0.5, 16, 16);
	GeometryGenerator::MeshData towerPlatformCylinder = geoGen.CreateCylinder(0.5, 0.5, 1.0, 20, 6);
	GeometryGenerator::MeshData towerCone = geoGen.CreateCone(0.5f, 1.0, 20, 16);


	// CN Tower
	//verticies
	UINT towerCylinderVertexOffset = 0;
	UINT towerWedgeVertexOffset = (UINT)towerCylinder.Vertices.size();
	UINT towerPlatformSphereVertexOffset = towerWedgeVertexOffset + (UINT)towerWedge.Vertices.size();
	UINT towerPlatformCylinderVertexOffset = towerPlatformSphereVertexOffset + (UINT)towerPlatformSphere.Vertices.size();
	UINT towerConeVertexOffset = towerPlatformCylinderVertexOffset + (UINT)towerPlatformCylinder.Vertices.size();

	//indicies
	UINT towerCylinderIndexOffset = 0;
	UINT towerWedgeIndexOffset = (UINT)towerCylinder.Indices32.size();
	UINT towerPlatformSphereIndexOffset = towerWedgeIndexOffset + (UINT)towerWedge.Indices32.size();
	UINT towerPlatformCylinderIndexOffset = towerPlatformSphereIndexOffset + (UINT)towerPlatformSphere.Indices32.size();
	UINT towerConeIndexOffset = towerPlatformCylinderIndexOffset + (UINT)towerPlatformCylinder.Indices32.size();

	//CN Tower Submeshes
	SubmeshGeometry towerCylinderSubmesh;
	towerCylinderSubmesh.IndexCount = (UINT)towerCylinder.Indices32.size();
	towerCylinderSubmesh.StartIndexLocation = towerCylinderIndexOffset;
	towerCylinderSubmesh.BaseVertexLocation = towerCylinderVertexOffset;

	SubmeshGeometry towerWedgeSubmesh;
	towerWedgeSubmesh.IndexCount = (UINT)towerWedge.Indices32.size();
	towerWedgeSubmesh.StartIndexLocation = towerWedgeIndexOffset;
	towerWedgeSubmesh.BaseVertexLocation = towerWedgeVertexOffset;

	SubmeshGeometry towerPlatformSphereSubmesh;
	towerPlatformSphereSubmesh.IndexCount = (UINT)towerPlatformSphere.Indices32.size();
	towerPlatformSphereSubmesh.StartIndexLocation = towerPlatformSphereIndexOffset;
	towerPlatformSphereSubmesh.BaseVertexLocation = towerPlatformSphereVertexOffset;

	SubmeshGeometry towerPlatformCylinderSubmesh;
	towerPlatformCylinderSubmesh.IndexCount = (UINT)towerPlatformCylinder.Indices32.size();
	towerPlatformCylinderSubmesh.StartIndexLocation = towerPlatformCylinderIndexOffset;
	towerPlatformCylinderSubmesh.BaseVertexLocation = towerPlatformCylinderVertexOffset;

	SubmeshGeometry towerConeSubmesh;
	towerConeSubmesh.IndexCount = (UINT)towerCone.Indices32.size();
	towerConeSubmesh.StartIndexLocation = towerConeIndexOffset;
	towerConeSubmesh.BaseVertexLocation = towerConeVertexOffset;

	auto totalVertexCount = /*box.Vertices.size();*/
		towerCylinder.Vertices.size() +
		towerWedge.Vertices.size() +
		towerPlatformSphere.Vertices.size() +
		towerPlatformCylinder.Vertices.size() +
		towerCone.Vertices.size()
		;

	std::vector<Vertex> vertices(totalVertexCount);
	UINT k = 0;
	//CN Tower
	for (size_t i = 0; i < towerCylinder.Vertices.size(); ++i, ++k)
	{
		auto& p = towerCylinder.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = towerCylinder.Vertices[i].Normal;
		vertices[k].TexC = towerCylinder.Vertices[i].TexC;
	}

	for (size_t i = 0; i < towerWedge.Vertices.size(); ++i, ++k)
	{
		auto& p = towerWedge.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = towerWedge.Vertices[i].Normal;
		vertices[k].TexC = towerWedge.Vertices[i].TexC;
	}

	for (size_t i = 0; i < towerPlatformSphere.Vertices.size(); ++i, ++k)
	{
		auto& p = towerPlatformSphere.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = towerPlatformSphere.Vertices[i].Normal;
		vertices[k].TexC = towerPlatformSphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < towerPlatformCylinder.Vertices.size(); ++i, ++k)
	{
		auto& p = towerPlatformCylinder.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = towerPlatformCylinder.Vertices[i].Normal;
		vertices[k].TexC = towerPlatformCylinder.Vertices[i].TexC;
	}

	for (size_t i = 0; i < towerCone.Vertices.size(); ++i, ++k)
	{
		auto& p = towerCone.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = towerCone.Vertices[i].Normal;
		vertices[k].TexC = towerCone.Vertices[i].TexC;
	}



	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;

	//CN Tower

	indices.insert(indices.end(), std::begin(towerCylinder.GetIndices16()), std::end(towerCylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(towerWedge.GetIndices16()), std::end(towerWedge.GetIndices16()));
	indices.insert(indices.end(), std::begin(towerPlatformSphere.GetIndices16()), std::end(towerPlatformSphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(towerPlatformCylinder.GetIndices16()), std::end(towerPlatformCylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(towerCone.GetIndices16()), std::end(towerCone.GetIndices16()));

	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "CNTowerGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	/*SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;*/

	geo->DrawArgs["towerCylinder"] = towerCylinderSubmesh;
	geo->DrawArgs["towerWedge"] = towerWedgeSubmesh;
	geo->DrawArgs["towerPlatformSphere"] = towerPlatformSphereSubmesh;
	geo->DrawArgs["towerPlatformCylinder"] = towerPlatformCylinderSubmesh;
	geo->DrawArgs["towerCone"] = towerConeSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void Game::BuildRogersCenter()
{
	GeometryGenerator geoGen;

	GeometryGenerator::MeshData rogersCylinder = geoGen.CreateCylinder(0.5, 0.5, 1.0f, 20, 16);
	GeometryGenerator::MeshData rogersDome = geoGen.CreateDome(0.5, 20, 20);


	//verticies
	// Rogers Center
	UINT rogersCylinderVertexOffset = 0;
	UINT rogersDomeVertexOffset = (UINT)rogersCylinder.Vertices.size();

	//indicies
	UINT rogersCylinderIndexOffset = 0;
	UINT rogersDomeIndexOffset = (UINT)rogersCylinder.Indices32.size();

	//Rogers Center
	SubmeshGeometry rogersCylinderSubmesh;
	rogersCylinderSubmesh.IndexCount = (UINT)rogersCylinder.Indices32.size();
	rogersCylinderSubmesh.StartIndexLocation = rogersCylinderIndexOffset;
	rogersCylinderSubmesh.BaseVertexLocation = rogersCylinderVertexOffset;

	SubmeshGeometry rogersDomeSubmesh;
	rogersDomeSubmesh.IndexCount = (UINT)rogersDome.Indices32.size();
	rogersDomeSubmesh.StartIndexLocation = rogersDomeIndexOffset;
	rogersDomeSubmesh.BaseVertexLocation = rogersDomeVertexOffset;


	auto totalVertexCount = /*box.Vertices.size();*/
		rogersCylinder.Vertices.size() +
		rogersDome.Vertices.size()
		;

	std::vector<Vertex> vertices(totalVertexCount);
	UINT k = 0;
	//CN Tower
	for (size_t i = 0; i < rogersCylinder.Vertices.size(); ++i, ++k)
	{
		auto& p = rogersCylinder.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = rogersCylinder.Vertices[i].Normal;
		vertices[k].TexC = rogersCylinder.Vertices[i].TexC;
	}

	for (size_t i = 0; i < rogersDome.Vertices.size(); ++i, ++k)
	{
		auto& p = rogersDome.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = rogersDome.Vertices[i].Normal;
		vertices[k].TexC = rogersDome.Vertices[i].TexC;
	}




	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;

	// Rogers Center
	indices.insert(indices.end(), std::begin(rogersCylinder.GetIndices16()), std::end(rogersCylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(rogersDome.GetIndices16()), std::end(rogersDome.GetIndices16()));

	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "RogersCenterGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;


	geo->DrawArgs["rogersCylinder"] = rogersCylinderSubmesh;
	geo->DrawArgs["rogersDome"] = rogersDomeSubmesh;

	mGeometries[geo->Name] = std::move(geo);

}

void Game::BuildBuildings()
{

	GeometryGenerator geoGen;

	GeometryGenerator::MeshData buildingBox = geoGen.CreateBox(1.0, 1.0, 1.0, 1);
	GeometryGenerator::MeshData buildingFlatPyramid = geoGen.CreateFlatToppedPyramid(1.0, 1.0, 1.0, 1);
	GeometryGenerator::MeshData buildingPyramid = geoGen.CreatePyramid(1.0, 1.0, 1.0, 1);


	//verticies
	UINT buildingBoxVertexOffset = 0;
	UINT buildingFlatPyramidVertexOffset = (UINT)buildingBox.Vertices.size();
	UINT buildingPyramidVertexOffset = buildingFlatPyramidVertexOffset + (UINT)buildingFlatPyramid.Vertices.size();

	//indicies
	UINT buildingBoxIndexOffset = 0;
	UINT buildingFlatPyramidIndexOffset = (UINT)buildingBox.Indices32.size();
	UINT buildingPyramidIndexOffset = buildingFlatPyramidIndexOffset + (UINT)buildingFlatPyramid.Indices32.size();


	//Buildings
	SubmeshGeometry buildingBoxSubmesh;
	buildingBoxSubmesh.IndexCount = (UINT)buildingBox.Indices32.size();
	buildingBoxSubmesh.StartIndexLocation = buildingBoxIndexOffset;
	buildingBoxSubmesh.BaseVertexLocation = buildingBoxVertexOffset;

	SubmeshGeometry buildingFlatPyramidSubmesh;
	buildingFlatPyramidSubmesh.IndexCount = (UINT)buildingFlatPyramid.Indices32.size();
	buildingFlatPyramidSubmesh.StartIndexLocation = buildingFlatPyramidIndexOffset;
	buildingFlatPyramidSubmesh.BaseVertexLocation = buildingFlatPyramidVertexOffset;

	SubmeshGeometry buildingPyramidSubmesh;
	buildingPyramidSubmesh.IndexCount = (UINT)buildingPyramid.Indices32.size();
	buildingPyramidSubmesh.StartIndexLocation = buildingPyramidIndexOffset;
	buildingPyramidSubmesh.BaseVertexLocation = buildingPyramidVertexOffset;


	auto totalVertexCount = /*box.Vertices.size();*/
		buildingBox.Vertices.size() +
		buildingFlatPyramid.Vertices.size() +
		buildingPyramid.Vertices.size()
		;

	std::vector<Vertex> vertices(totalVertexCount);
	UINT k = 0;
	//CN Tower
	for (size_t i = 0; i < buildingBox.Vertices.size(); ++i, ++k)
	{
		auto& p = buildingBox.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = buildingBox.Vertices[i].Normal;
		vertices[k].TexC = buildingBox.Vertices[i].TexC;
	}

	for (size_t i = 0; i < buildingFlatPyramid.Vertices.size(); ++i, ++k)
	{
		auto& p = buildingFlatPyramid.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = buildingFlatPyramid.Vertices[i].Normal;
		vertices[k].TexC = buildingFlatPyramid.Vertices[i].TexC;
	}
	for (size_t i = 0; i < buildingPyramid.Vertices.size(); ++i, ++k)
	{
		auto& p = buildingPyramid.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = buildingPyramid.Vertices[i].Normal;
		vertices[k].TexC = buildingPyramid.Vertices[i].TexC;
	}




	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices;

	// Buildings
	indices.insert(indices.end(), std::begin(buildingBox.GetIndices16()), std::end(buildingBox.GetIndices16()));
	indices.insert(indices.end(), std::begin(buildingFlatPyramid.GetIndices16()), std::end(buildingFlatPyramid.GetIndices16()));
	indices.insert(indices.end(), std::begin(buildingPyramid.GetIndices16()), std::end(buildingPyramid.GetIndices16()));

	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "buildingsGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;


	geo->DrawArgs["buildingBox"] = buildingBoxSubmesh;
	geo->DrawArgs["buildingFlatPyramid"] = buildingFlatPyramidSubmesh;
	geo->DrawArgs["buildingPyramid"] = buildingPyramidSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void Game::BuildWavesGeometry()
{
	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

	// Iterate over each quad.
	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void Game::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

	std::vector<Vertex> vertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = box.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["box"] = submesh;

	mGeometries["boxGeo"] = std::move(geo);
}

void Game::BuildDiamondGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData diamond = geoGen.CreateDiamond(1.0f, 0.5f, 8.0f);

	std::vector<Vertex> vertices(diamond.Vertices.size());
	for (size_t i = 0; i < diamond.Vertices.size(); ++i)
	{
		auto& p = diamond.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = diamond.Vertices[i].Normal;
		vertices[i].TexC = diamond.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = diamond.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "diamondGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["diamond"] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void Game::BuildTreeSpritesGeometry()
{
	//step5
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 16;
	std::array<TreeSpriteVertex, 16> vertices;

	// make a line of 6 trees
	for (int i = 0; i < 6; i++)
	{
		vertices[i].Pos = XMFLOAT3((i + 2) * scaleFactor, 0.5 * scaleFactor, MathHelper::RandF(-1.9f, -1.2f) * scaleFactor);
		
		vertices[i].Size = XMFLOAT2(1.0 * scaleFactor, 1.0f * scaleFactor);
	}

	// make a line of 6 trees
	for (int i = 6; i < 12; i++)
	{
		vertices[i].Pos = XMFLOAT3((i - 5) * scaleFactor, 0.5 * scaleFactor, MathHelper::RandF(-2.5, -2.0) * scaleFactor);
		vertices[i].Size = XMFLOAT2(1.0 * scaleFactor, 1.0f * scaleFactor);
	}


	vertices[12].Pos = XMFLOAT3(-4 * scaleFactor, 0.5 * scaleFactor, -2.5 * scaleFactor);
	vertices[12].Size = XMFLOAT2(1.0 * scaleFactor, 1.0f * scaleFactor);

	vertices[13].Pos = XMFLOAT3(-5.8 * scaleFactor, 0.5 * scaleFactor, -2.7 * scaleFactor);
	vertices[13].Size = XMFLOAT2(1.0 * scaleFactor, 1.0f * scaleFactor);

	vertices[14].Pos = XMFLOAT3(0 * scaleFactor, 0.5 * scaleFactor, -2 * scaleFactor);
	vertices[14].Size = XMFLOAT2(1.0 * scaleFactor, 1.0f * scaleFactor);

	vertices[15].Pos = XMFLOAT3(4.4 * scaleFactor, 0.5 * scaleFactor, -3 * scaleFactor);
	vertices[15].Size = XMFLOAT2(1.0 * scaleFactor, 1.0f * scaleFactor);

	////make 16 randomly placed trees
	//for (UINT i = 0; i < treeCount; ++i)
	//{
	//	float x = MathHelper::RandF(-10.0f, 10.0f);
	//	float z = MathHelper::RandF(-2.0f, 5.0f);
	//	float y = 0;

	//	// Move tree slightly above land height.
	//	y += 0.5f;

	//	vertices[i].Pos = XMFLOAT3(x, y, z);
	//	vertices[i].Size = XMFLOAT2(1.0, 1.0f);
	//}

	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}

void Game::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;

	//there is abug with F2 key that is supposed to turn on the multisampling!
//Set4xMsaaState(true);
	//m4xMsaaState = true;

	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

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

	//transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	//step1
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void Game::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
	}
}

void Game::BuildMaterials()
{
	int matIndex = 0;
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = matIndex;
	grass->DiffuseSrvHeapIndex = matIndex++;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	// This is not a good water material definition, but we do not have all the rendering
	// tools we need (transparency, environment reflection), so we fake it for now.
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = matIndex;
	water->DiffuseSrvHeapIndex = matIndex++;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	auto  roof = std::make_unique<Material>();
	roof->Name = "roof";
	roof->MatCBIndex = matIndex;
	roof->DiffuseSrvHeapIndex = matIndex++;
	roof->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	roof->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	roof->Roughness = 0.25f;

	auto building = std::make_unique<Material>();
	building->Name = "building";
	building->MatCBIndex = matIndex;
	building->DiffuseSrvHeapIndex = matIndex++;
	building->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	building->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	building->Roughness = 0.125f;

	auto concrete = std::make_unique<Material>();
	concrete->Name = "concrete";
	concrete->MatCBIndex = matIndex;
	concrete->DiffuseSrvHeapIndex = matIndex++;
	concrete->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	concrete->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	concrete->Roughness = 0.125f;

	auto brick = std::make_unique<Material>();
	brick->Name = "brick";
	brick->MatCBIndex = matIndex;
	brick->DiffuseSrvHeapIndex = matIndex++;
	brick->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	brick->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	brick->Roughness = 0.125f;

	auto wirefence = std::make_unique<Material>();
	wirefence->Name = "wirefence";
	wirefence->MatCBIndex = matIndex;
	wirefence->DiffuseSrvHeapIndex = matIndex++;
	wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	wirefence->Roughness = 0.25f;

	auto road = std::make_unique<Material>();
	road->Name = "road";
	road->MatCBIndex = matIndex;
	road->DiffuseSrvHeapIndex = matIndex++;
	road->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	road->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	road->Roughness = 0.125f;

	auto roadI = std::make_unique<Material>();
	roadI->Name = "roadI";
	roadI->MatCBIndex = matIndex;
	roadI->DiffuseSrvHeapIndex = matIndex++;
	roadI->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	roadI->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	roadI->Roughness = 0.125f;


	auto treeSprites = std::make_unique<Material>();
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = matIndex;
	treeSprites->DiffuseSrvHeapIndex = matIndex++;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials["roof"] = std::move(roof);
	mMaterials["building"] = std::move(building);
	mMaterials["concrete"] = std::move(concrete);
	mMaterials["brick"] = std::move(brick);
	mMaterials["wirefence"] = std::move(wirefence);
	mMaterials["road"] = std::move(road);
	mMaterials["roadI"] = std::move(roadI);
	mMaterials["treeSprites"] = std::move(treeSprites);

}

void Game::BuildRenderPyra(float x, float y, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex)
{
	auto Pyramid1Ritem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&Pyramid1Ritem->World, XMMatrixScaling(scalex * scaleFactor, scaley * scaleFactor, scalez * scaleFactor) * XMMatrixTranslation(x * scaleFactor, y * scaleFactor, z * scaleFactor));/// can choose your scaling here
	Pyramid1Ritem->ObjCBIndex = ObjCBIndex++; /// have to change the object index for it to work
	Pyramid1Ritem->Mat = mMaterials["roof"].get();
	Pyramid1Ritem->Geo = mGeometries["buildingsGeo"].get();
	Pyramid1Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Pyramid1Ritem->IndexCount = Pyramid1Ritem->Geo->DrawArgs["buildingPyramid"].IndexCount;
	Pyramid1Ritem->StartIndexLocation = Pyramid1Ritem->Geo->DrawArgs["buildingPyramid"].StartIndexLocation;
	Pyramid1Ritem->BaseVertexLocation = Pyramid1Ritem->Geo->DrawArgs["buildingPyramid"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(Pyramid1Ritem.get());
	mAllRitems.push_back(std::move(Pyramid1Ritem));
}

void Game::BuildRenderFlatTopPyra(float x, float y, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex)
{
	auto FlatPyramid2Ritem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&FlatPyramid2Ritem->World, XMMatrixScaling(scalex * scaleFactor, scaley * scaleFactor, scalez * scaleFactor) * XMMatrixTranslation(x * scaleFactor, y * scaleFactor, z * scaleFactor));/// can choose your scaling here
	FlatPyramid2Ritem->ObjCBIndex = ObjCBIndex++; /// have to change the object index for it to work
	FlatPyramid2Ritem->Mat = mMaterials["roof"].get();
	FlatPyramid2Ritem->Geo = mGeometries["buildingsGeo"].get();
	FlatPyramid2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FlatPyramid2Ritem->IndexCount = FlatPyramid2Ritem->Geo->DrawArgs["buildingFlatPyramid"].IndexCount;
	FlatPyramid2Ritem->StartIndexLocation = FlatPyramid2Ritem->Geo->DrawArgs["buildingFlatPyramid"].StartIndexLocation;
	FlatPyramid2Ritem->BaseVertexLocation = FlatPyramid2Ritem->Geo->DrawArgs["buildingFlatPyramid"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(FlatPyramid2Ritem.get());
	mAllRitems.push_back(std::move(FlatPyramid2Ritem));
}

void Game::BuildRenderBuilding(float x, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex)
{
	auto Building1Ritem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&Building1Ritem->World, XMMatrixScaling(scalex * scaleFactor, scaley * scaleFactor, scalez * scaleFactor) * XMMatrixTranslation(x * scaleFactor, scaley / 2.0f * scaleFactor, z * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&Building1Ritem->TexTransform, XMMatrixScaling(scalex, scaley, scalez));
	Building1Ritem->ObjCBIndex = ObjCBIndex++; /// have to change the object index for it to work
	switch (MathHelper::Rand(0, 1))
	{
	case 0:
		Building1Ritem->Mat = mMaterials["brick"].get();
		break;
	default:
		Building1Ritem->Mat = mMaterials["building"].get();
		break;
	}
	//Building1Ritem->Mat = mMaterials["building"].get();
	Building1Ritem->Geo = mGeometries["buildingsGeo"].get();
	Building1Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Building1Ritem->IndexCount = Building1Ritem->Geo->DrawArgs["buildingBox"].IndexCount;
	Building1Ritem->StartIndexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
	Building1Ritem->BaseVertexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(Building1Ritem.get());
	mAllRitems.push_back(std::move(Building1Ritem));

	switch (MathHelper::Rand(0, 2))
	{
	case 0:
		BuildRenderFlatTopPyra(x, scaley + (scaley * 0.05), z, scalex, scaley * 0.1, scalez, ObjCBIndex);
		break;
	case 1:
		BuildRenderPyra(x, scaley + (scaley * 0.05), z, scalex, scaley * 0.1, scalez, ObjCBIndex);
		break;
	case 2:
	{
		auto boxRitem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(scalex * scaleFactor, 0.2 * scaleFactor, scalez * scaleFactor) * XMMatrixTranslation(x * scaleFactor, (scaley + 0.1) * scaleFactor, z * scaleFactor));/// can choose your scaling here
		XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1, 1, 1));
		boxRitem->ObjCBIndex = ObjCBIndex++; /// have to change the object index for it to work
		boxRitem->Mat = mMaterials["concrete"].get();
		boxRitem->Geo = mGeometries["buildingsGeo"].get();
		boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs["buildingBox"].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
		mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
		mAllRitems.push_back(std::move(boxRitem));
		break;
	}
	default:
		//BuildRenderFlatTopPyra(x, scaley + (scaley * 0.05), z, scalex, scaley * 0.1, scalez, ObjCBIndex);
		break;
	}

	BoundingBox bounds;
	XMFLOAT3 centerf3(x * scaleFactor, scaley / 2.0f * scaleFactor, z * scaleFactor);
	XMVECTOR center = XMLoadFloat3(&centerf3);
	XMFLOAT3 extentsf3(scalex * scaleFactor /2  , scaley * scaleFactor /2, scalez * scaleFactor /2);
	XMVECTOR extents = XMLoadFloat3(&extentsf3);
	XMStoreFloat3(&bounds.Center, center);
	XMStoreFloat3(&bounds.Extents, extents);
	mBoundingBoxes.push_back(bounds);
	
}

void Game::BuildRenderRoad(float x, float z, float length, float width, float angle, UINT& ObjCBIndex)
{
	auto Building1Ritem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&Building1Ritem->World, XMMatrixScaling(width * scaleFactor, 0.02 * scaleFactor, length * scaleFactor) * XMMatrixRotationY(angle * 0.0174533) * XMMatrixTranslation(x * scaleFactor, 0.01 * scaleFactor, z * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&Building1Ritem->TexTransform, XMMatrixScaling(2.0f, length * 2.0f, 1.0f));
	Building1Ritem->ObjCBIndex = ObjCBIndex++; /// have to change the object index for it to work
	Building1Ritem->Mat = mMaterials["road"].get();
	Building1Ritem->Geo = mGeometries["buildingsGeo"].get();
	Building1Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Building1Ritem->IndexCount = Building1Ritem->Geo->DrawArgs["buildingBox"].IndexCount;
	Building1Ritem->StartIndexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
	Building1Ritem->BaseVertexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(Building1Ritem.get());
	mAllRitems.push_back(std::move(Building1Ritem));
}

void Game::BuildRenderIntersection(float x, float z, float scalex, float scalez, UINT& ObjCBIndex)
{
	auto Building1Ritem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&Building1Ritem->World, XMMatrixScaling(scalex * scaleFactor, 0.03 * scaleFactor, scalez * scaleFactor) * XMMatrixTranslation(x * scaleFactor, 0.01 * scaleFactor, z * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&Building1Ritem->TexTransform, XMMatrixScaling(1.0, 1.0, 1.0));
	Building1Ritem->ObjCBIndex = ObjCBIndex++; /// have to change the object index for it to work
	Building1Ritem->Mat = mMaterials["roadI"].get();
	Building1Ritem->Geo = mGeometries["buildingsGeo"].get();
	Building1Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Building1Ritem->IndexCount = Building1Ritem->Geo->DrawArgs["buildingBox"].IndexCount;
	Building1Ritem->StartIndexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
	Building1Ritem->BaseVertexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(Building1Ritem.get());
	mAllRitems.push_back(std::move(Building1Ritem));

}

void Game::BuildRenderItems()
{
	UINT objCBIndex = 0;

	//ocean
	auto wavesRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&wavesRitem->World, XMMatrixScaling(1.0f * scaleFactor, 0.02f , 1.0f * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor,0 * scaleFactor, -9.85 * scaleFactor) );
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->ObjCBIndex = objCBIndex++;
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());
	mAllRitems.push_back(std::move(wavesRitem));

	BoundingBox oceanBounds;
	XMFLOAT3 oceanCenterf3(0.0f * scaleFactor, 0 * scaleFactor, -12 * scaleFactor);
	XMVECTOR oceanCenter = XMLoadFloat3(&oceanCenterf3);
	XMFLOAT3 oceanExtentsf3(10 * scaleFactor, 10.0f, 7.5 * scaleFactor);
	XMVECTOR oceanExtents = XMLoadFloat3(&oceanExtentsf3);
	XMStoreFloat3(&oceanBounds.Center, oceanCenter);
	XMStoreFloat3(&oceanBounds.Extents, oceanExtents);
	mBoundingBoxes.push_back(oceanBounds);

	//the ground under the ocean
	auto gridRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&gridRitem->World, XMMatrixScaling(1.0f * scaleFactor, 0.02f * scaleFactor, 1.0f * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, -0.2f * scaleFactor, 0.0f * scaleFactor));
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->ObjCBIndex = objCBIndex++;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	mAllRitems.push_back(std::move(gridRitem));

	// land for the city
	auto groundRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&groundRitem->World, XMMatrixScaling(1.0f * scaleFactor, 1.0f * scaleFactor, 1.0f * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, -0.1 * scaleFactor, 5.0f * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&groundRitem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0f));
	groundRitem->ObjCBIndex = objCBIndex++;
	groundRitem->Mat = mMaterials["grass"].get();
	groundRitem->Geo = mGeometries["groundGeo"].get();
	groundRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	groundRitem->IndexCount = groundRitem->Geo->DrawArgs["ground"].IndexCount;
	groundRitem->StartIndexLocation = groundRitem->Geo->DrawArgs["ground"].StartIndexLocation;
	groundRitem->BaseVertexLocation = groundRitem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(groundRitem.get());
	mAllRitems.push_back(std::move(groundRitem));

	//CN Tower
	float towerScaleY = 8.0;
	auto towerCylinderRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerCylinderRitem->World, XMMatrixScaling(1.0f * scaleFactor, towerScaleY * scaleFactor, 1.0f * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, towerScaleY*0.5 * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	towerCylinderRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerCylinderRitem->Mat = mMaterials["concrete"].get();
	towerCylinderRitem->Geo = mGeometries["CNTowerGeo"].get();
	towerCylinderRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerCylinderRitem->IndexCount = towerCylinderRitem->Geo->DrawArgs["towerCylinder"].IndexCount;
	towerCylinderRitem->StartIndexLocation = towerCylinderRitem->Geo->DrawArgs["towerCylinder"].StartIndexLocation;
	towerCylinderRitem->BaseVertexLocation = towerCylinderRitem->Geo->DrawArgs["towerCylinder"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerCylinderRitem.get());
	mAllRitems.push_back(std::move(towerCylinderRitem));

	float wedgeRadius = 0.57;
	auto towerWedgeRitem1 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerWedgeRitem1->World, XMMatrixScaling(0.3 * scaleFactor, towerScaleY*0.8 * scaleFactor, 0.3 * scaleFactor) * XMMatrixRotationY((0 - 90) * 0.0174533) * XMMatrixTranslation(sin(0 * 0.0174533) * wedgeRadius * scaleFactor, 2 * scaleFactor, cos(0 * 0.0174533) * wedgeRadius * scaleFactor));/// can choose your scaling here
	towerWedgeRitem1->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerWedgeRitem1->Mat = mMaterials["concrete"].get();
	towerWedgeRitem1->Geo = mGeometries["CNTowerGeo"].get();
	towerWedgeRitem1->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerWedgeRitem1->IndexCount = towerWedgeRitem1->Geo->DrawArgs["towerWedge"].IndexCount;
	towerWedgeRitem1->StartIndexLocation = towerWedgeRitem1->Geo->DrawArgs["towerWedge"].StartIndexLocation;
	towerWedgeRitem1->BaseVertexLocation = towerWedgeRitem1->Geo->DrawArgs["towerWedge"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerWedgeRitem1.get());
	mAllRitems.push_back(std::move(towerWedgeRitem1));

	auto towerWedgeRitem2 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerWedgeRitem2->World, XMMatrixScaling(0.3 * scaleFactor, towerScaleY * 0.8 * scaleFactor, 0.3 * scaleFactor) * XMMatrixRotationY((120 -90) * 0.0174533) * XMMatrixTranslation(sin(120 * 0.0174533) * wedgeRadius * scaleFactor, 2 * scaleFactor, cos(120 * 0.0174533) * wedgeRadius * scaleFactor));/// can choose your scaling here
	towerWedgeRitem2->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerWedgeRitem2->Mat = mMaterials["concrete"].get();
	towerWedgeRitem2->Geo = mGeometries["CNTowerGeo"].get();
	towerWedgeRitem2->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerWedgeRitem2->IndexCount = towerWedgeRitem2->Geo->DrawArgs["towerWedge"].IndexCount;
	towerWedgeRitem2->StartIndexLocation = towerWedgeRitem2->Geo->DrawArgs["towerWedge"].StartIndexLocation;
	towerWedgeRitem2->BaseVertexLocation = towerWedgeRitem2->Geo->DrawArgs["towerWedge"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerWedgeRitem2.get());
	mAllRitems.push_back(std::move(towerWedgeRitem2));

	auto towerWedgeRitem3 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerWedgeRitem3->World, XMMatrixScaling(0.3 * scaleFactor, towerScaleY * 0.8 * scaleFactor, 0.3 * scaleFactor) * XMMatrixRotationY((240 -90) * 0.0174533) * XMMatrixTranslation(sin(240 * 0.0174533) * wedgeRadius * scaleFactor, 2 * scaleFactor, cos(240 * 0.0174533) * wedgeRadius * scaleFactor));/// can choose your scaling here
	towerWedgeRitem3->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerWedgeRitem3->Mat = mMaterials["concrete"].get();
	towerWedgeRitem3->Geo = mGeometries["CNTowerGeo"].get();
	towerWedgeRitem3->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerWedgeRitem3->IndexCount = towerWedgeRitem3->Geo->DrawArgs["towerWedge"].IndexCount;
	towerWedgeRitem3->StartIndexLocation = towerWedgeRitem3->Geo->DrawArgs["towerWedge"].StartIndexLocation;
	towerWedgeRitem3->BaseVertexLocation = towerWedgeRitem3->Geo->DrawArgs["towerWedge"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerWedgeRitem3.get());
	mAllRitems.push_back(std::move(towerWedgeRitem3));

	auto towerPlatformSphereRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerPlatformSphereRitem->World, XMMatrixScaling(2 * scaleFactor, 0.5 * scaleFactor, 2 * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, towerScaleY * 0.7 * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	towerPlatformSphereRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerPlatformSphereRitem->Mat = mMaterials["concrete"].get();
	towerPlatformSphereRitem->Geo = mGeometries["CNTowerGeo"].get();
	towerPlatformSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerPlatformSphereRitem->IndexCount = towerPlatformSphereRitem->Geo->DrawArgs["towerPlatformSphere"].IndexCount;
	towerPlatformSphereRitem->StartIndexLocation = towerPlatformSphereRitem->Geo->DrawArgs["towerPlatformSphere"].StartIndexLocation;
	towerPlatformSphereRitem->BaseVertexLocation = towerPlatformSphereRitem->Geo->DrawArgs["towerPlatformSphere"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerPlatformSphereRitem.get());
	mAllRitems.push_back(std::move(towerPlatformSphereRitem));

	auto towerPlatformCylinderRitem1 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerPlatformCylinderRitem1->World, XMMatrixScaling(2 * scaleFactor, 1 * scaleFactor, 2 * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, towerScaleY * 0.85 * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&towerPlatformCylinderRitem1->TexTransform, XMMatrixScaling(2.0, 2.0, 2.0));
	towerPlatformCylinderRitem1->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerPlatformCylinderRitem1->Mat = mMaterials["building"].get();
	towerPlatformCylinderRitem1->Geo = mGeometries["CNTowerGeo"].get();
	towerPlatformCylinderRitem1->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerPlatformCylinderRitem1->IndexCount = towerPlatformCylinderRitem1->Geo->DrawArgs["towerPlatformCylinder"].IndexCount;
	towerPlatformCylinderRitem1->StartIndexLocation = towerPlatformCylinderRitem1->Geo->DrawArgs["towerPlatformCylinder"].StartIndexLocation;
	towerPlatformCylinderRitem1->BaseVertexLocation = towerPlatformCylinderRitem1->Geo->DrawArgs["towerPlatformCylinder"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerPlatformCylinderRitem1.get());
	mAllRitems.push_back(std::move(towerPlatformCylinderRitem1));

	auto towerplatformcylinderRitem1Top = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerplatformcylinderRitem1Top->World, XMMatrixScaling(2.2 * scaleFactor, 0.1 * scaleFactor, 2.2 * scaleFactor)* XMMatrixTranslation(0.0f * scaleFactor, ((towerScaleY * 0.85) + 0.5) * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&towerplatformcylinderRitem1Top->TexTransform, XMMatrixScaling(2.0, 2.0, 2.0));
	towerplatformcylinderRitem1Top->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerplatformcylinderRitem1Top->Mat = mMaterials["concrete"].get();
	towerplatformcylinderRitem1Top->Geo = mGeometries["CNTowerGeo"].get();
	towerplatformcylinderRitem1Top->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerplatformcylinderRitem1Top->IndexCount = towerplatformcylinderRitem1Top->Geo->DrawArgs["towerPlatformCylinder"].IndexCount;
	towerplatformcylinderRitem1Top->StartIndexLocation = towerplatformcylinderRitem1Top->Geo->DrawArgs["towerPlatformCylinder"].StartIndexLocation;
	towerplatformcylinderRitem1Top->BaseVertexLocation = towerplatformcylinderRitem1Top->Geo->DrawArgs["towerPlatformCylinder"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerplatformcylinderRitem1Top.get());
	mAllRitems.push_back(std::move(towerplatformcylinderRitem1Top));

	auto towerplatformcylinderRitem1bott = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerplatformcylinderRitem1bott->World, XMMatrixScaling(2.2 * scaleFactor, 0.1 * scaleFactor, 2.2 * scaleFactor)* XMMatrixTranslation(0.0f * scaleFactor, ((towerScaleY * 0.85) - 0.5) * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&towerplatformcylinderRitem1bott->TexTransform, XMMatrixScaling(2.0, 2.0, 2.0));
	towerplatformcylinderRitem1bott->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerplatformcylinderRitem1bott->Mat = mMaterials["concrete"].get();
	towerplatformcylinderRitem1bott->Geo = mGeometries["CNTowerGeo"].get();
	towerplatformcylinderRitem1bott->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerplatformcylinderRitem1bott->IndexCount = towerplatformcylinderRitem1bott->Geo->DrawArgs["towerPlatformCylinder"].IndexCount;
	towerplatformcylinderRitem1bott->StartIndexLocation = towerplatformcylinderRitem1bott->Geo->DrawArgs["towerPlatformCylinder"].StartIndexLocation;
	towerplatformcylinderRitem1bott->BaseVertexLocation = towerplatformcylinderRitem1bott->Geo->DrawArgs["towerPlatformCylinder"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerplatformcylinderRitem1bott.get());
	mAllRitems.push_back(std::move(towerplatformcylinderRitem1bott));

	auto towerPlatformCylinderRitem2 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerPlatformCylinderRitem2->World, XMMatrixScaling(1 * scaleFactor, 0.3 * scaleFactor, 1 * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, towerScaleY * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	towerPlatformCylinderRitem2->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerPlatformCylinderRitem2->Mat = mMaterials["concrete"].get();
	towerPlatformCylinderRitem2->Geo = mGeometries["CNTowerGeo"].get();
	towerPlatformCylinderRitem2->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerPlatformCylinderRitem2->IndexCount = towerPlatformCylinderRitem2->Geo->DrawArgs["towerPlatformCylinder"].IndexCount;
	towerPlatformCylinderRitem2->StartIndexLocation = towerPlatformCylinderRitem2->Geo->DrawArgs["towerPlatformCylinder"].StartIndexLocation;
	towerPlatformCylinderRitem2->BaseVertexLocation = towerPlatformCylinderRitem2->Geo->DrawArgs["towerPlatformCylinder"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerPlatformCylinderRitem2.get());
	mAllRitems.push_back(std::move(towerPlatformCylinderRitem2));

	auto towerConeRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&towerConeRitem->World, XMMatrixScaling(0.5 * scaleFactor, towerScaleY*0.5 * scaleFactor, 0.5 * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, towerScaleY * 1.25 * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	towerConeRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	towerConeRitem->Mat = mMaterials["concrete"].get();
	towerConeRitem->Geo = mGeometries["CNTowerGeo"].get();
	towerConeRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	towerConeRitem->IndexCount = towerConeRitem->Geo->DrawArgs["towerCone"].IndexCount;
	towerConeRitem->StartIndexLocation = towerConeRitem->Geo->DrawArgs["towerCone"].StartIndexLocation;
	towerConeRitem->BaseVertexLocation = towerConeRitem->Geo->DrawArgs["towerCone"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(towerConeRitem.get());
	mAllRitems.push_back(std::move(towerConeRitem));

	

	BoundingBox CNTowerBounds;
	XMFLOAT3 CNTowerCenterf3(0.0f * scaleFactor, towerScaleY*0.5 * scaleFactor, 0.0f * scaleFactor);
	XMVECTOR CNTowerCenter = XMLoadFloat3(&CNTowerCenterf3);
	XMFLOAT3 CNTowerExtentsf3(1.0f * scaleFactor/2, towerScaleY * scaleFactor/2, 1.0f * scaleFactor/2);
	XMVECTOR CNTowerExtents = XMLoadFloat3(&CNTowerExtentsf3);
	XMStoreFloat3(&CNTowerBounds.Center, CNTowerCenter);
	XMStoreFloat3(&CNTowerBounds.Extents, CNTowerExtents);
	mBoundingBoxes.push_back(CNTowerBounds);

	//Rogers Center
	auto rogersCylinderRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&rogersCylinderRitem->World, XMMatrixScaling(4 * scaleFactor, 1.0f * scaleFactor, 4 * scaleFactor) * XMMatrixTranslation(-5 * scaleFactor, 0.5 * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	rogersCylinderRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	rogersCylinderRitem->Mat = mMaterials["concrete"].get();
	rogersCylinderRitem->Geo = mGeometries["RogersCenterGeo"].get();
	rogersCylinderRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rogersCylinderRitem->IndexCount = rogersCylinderRitem->Geo->DrawArgs["rogersCylinder"].IndexCount;
	rogersCylinderRitem->StartIndexLocation = rogersCylinderRitem->Geo->DrawArgs["rogersCylinder"].StartIndexLocation;
	rogersCylinderRitem->BaseVertexLocation = rogersCylinderRitem->Geo->DrawArgs["rogersCylinder"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(rogersCylinderRitem.get());
	mAllRitems.push_back(std::move(rogersCylinderRitem));

	auto rogersDomeRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&rogersDomeRitem->World, XMMatrixScaling(3.5 * scaleFactor, 2 * scaleFactor, 3.5 * scaleFactor) * XMMatrixTranslation(-5 * scaleFactor, 1 * scaleFactor, 0.0f * scaleFactor));/// can choose your scaling here
	rogersDomeRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	rogersDomeRitem->Mat = mMaterials["concrete"].get();
	rogersDomeRitem->Geo = mGeometries["RogersCenterGeo"].get();
	rogersDomeRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rogersDomeRitem->IndexCount = rogersDomeRitem->Geo->DrawArgs["rogersDome"].IndexCount;
	rogersDomeRitem->StartIndexLocation = rogersDomeRitem->Geo->DrawArgs["rogersDome"].StartIndexLocation;
	rogersDomeRitem->BaseVertexLocation = rogersDomeRitem->Geo->DrawArgs["rogersDome"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(rogersDomeRitem.get());
	mAllRitems.push_back(std::move(rogersDomeRitem));

	BoundingSphere RogersCenterBounds;
	XMFLOAT3 RogersCenterCenterf3(-5 * scaleFactor, 0.5 * scaleFactor, 0.0f * scaleFactor);
	XMVECTOR RogersCenterCenter = XMLoadFloat3(&RogersCenterCenterf3);
	XMStoreFloat3(&RogersCenterBounds.Center, RogersCenterCenter);
	RogersCenterBounds.Radius = 2.0f * scaleFactor;
	mBoundingSpheres.push_back(RogersCenterBounds);


	//Buildings

	//    location( x, z, scalex, scaley, scalez)

	// front middle building
	BuildRenderBuilding(-2, -2, 1, 2, 1, objCBIndex);

	//far front left
	BuildRenderBuilding(-8, -2, 1, 2, 1, objCBIndex);

	//far left back
	BuildRenderBuilding(-8, 1.5, 1, 3, 2, objCBIndex);

	// right side front left
	BuildRenderBuilding(2.5, 0, 1, 4, 1, objCBIndex);

	//right side back left*
	BuildRenderBuilding(2, 2, 1, 2, 1, objCBIndex);

	// right side middle front
	BuildRenderBuilding(5, 0, 2, 3, 1, objCBIndex);

	//right side back second *
	BuildRenderBuilding(3.5, 2, 1, 2, 1, objCBIndex);

	// right side back third
	BuildRenderBuilding(6, 2, 1, 3.4, 1, objCBIndex);

	// right side front last
	BuildRenderBuilding(7.5, 0, 1.5, 3, 1, objCBIndex);

	//right side back last
	BuildRenderBuilding(8, 2, 1, 3.6, 1, objCBIndex);

	// render background buildings

	for (int i = 0; i < 4; i++) // 4 rows of randomsized buildings
	{
		for (int k = 0; k < 6; k++) // 6 columns of randomized buildings
		{
			float x = 0;
			float y = ((rand()%20)/5.0f)+(1.0f);
			float z = 0;
			switch (i)
			{
			case 0:
				z = 4.5;
				break;
			case 1:
				z = 7.5;
				break;
			case 2:
				z = 10.5;
				break;
			case 3:
				z = 13.5;
				break;
			}
			switch (k)
			{
			case 0:
				// > 7.15
				x = 8.25;
				BuildRenderBuilding(x, z, 1.25, y, 1.25, objCBIndex);
				break;
			case 1:

				// between 7.15 & 4.75
				x = 6;
				BuildRenderBuilding(x, z, 1.25, y, 1.25, objCBIndex);
				break;
			case 2: 
				// one or two
				// between 4.75 & 0.75
				// 2 && 3.5
				if (rand() % 2)
				{
					x = (4.75 + 0.75) / 2;
					BuildRenderBuilding(x, z, 2, y, 1.25, objCBIndex);
				}
				else
				{
					x = (4.75 + 0.75) / 2;
					BuildRenderBuilding((x + 4.25) / 2, z, 1.25, y, 1.25, objCBIndex);
					BuildRenderBuilding((x + 1.25) / 2, z, 1.25, y, 1.25, objCBIndex);
				}
				break;
			case 3:
				// one or two
				// between 0.75 & -3.75
				if (rand() % 2)
				{
					x = (-3.75 + 0.75) / 2;
					BuildRenderBuilding(x, z, 2, y, 1.25, objCBIndex);
				}
				else
				{
					x = (-3.75 + 0.75) / 2;
					BuildRenderBuilding((x - 3.25) / 2, z, 1.25, y, 1.25, objCBIndex);
					BuildRenderBuilding((x + 0.25) / 2, z, 1.25, y, 1.25, objCBIndex);
				}
				break;
			case 4:
				// only1
				// between -3.75 & -6.75
				x = -5.25;
				BuildRenderBuilding(x, z, 1.25, y, 1.25, objCBIndex);
				break;
			case 5:
				// only1
				// < - 6.75
				x = -8;
				BuildRenderBuilding(x, z, 1.25, y, 1.25, objCBIndex);
				break;
			default:
				break;
			}
		}
	}

	//building under construction
	//building base
	auto Building1Ritem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&Building1Ritem->World, XMMatrixScaling(1.5 * scaleFactor, 1.5 * scaleFactor, 1.2 * scaleFactor) * XMMatrixTranslation(9 * scaleFactor, 1.5 / 2.0f * scaleFactor, -4 * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&Building1Ritem->TexTransform, XMMatrixScaling(1.5, 1.5, 1.2));
	Building1Ritem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	Building1Ritem->Mat = mMaterials["building"].get();
	Building1Ritem->Geo = mGeometries["buildingsGeo"].get();
	Building1Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Building1Ritem->IndexCount = Building1Ritem->Geo->DrawArgs["buildingBox"].IndexCount;
	Building1Ritem->StartIndexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
	Building1Ritem->BaseVertexLocation = Building1Ritem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(Building1Ritem.get());
	mAllRitems.push_back(std::move(Building1Ritem));
	//building roof
	auto roofRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&roofRitem->World, XMMatrixScaling(1.5 * scaleFactor, 0.1 * scaleFactor, 1.2 * scaleFactor) * XMMatrixTranslation(9 * scaleFactor, (1.5 + 0.05) * scaleFactor, -4 * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&roofRitem->TexTransform, XMMatrixScaling(1, 1, 1));
	roofRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	roofRitem->Mat = mMaterials["concrete"].get();
	roofRitem->Geo = mGeometries["buildingsGeo"].get();
	roofRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	roofRitem->IndexCount = roofRitem->Geo->DrawArgs["buildingBox"].IndexCount;
	roofRitem->StartIndexLocation = roofRitem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
	roofRitem->BaseVertexLocation = roofRitem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(roofRitem.get());
	mAllRitems.push_back(std::move(roofRitem));
	//crane vertical portion
	auto craneRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&craneRitem->World, XMMatrixScaling(0.35 * scaleFactor, 2.2 * scaleFactor, 0.35 * scaleFactor) * XMMatrixTranslation(9 * scaleFactor, (1.5 + 2.2 / 2.0f) * scaleFactor, -4 * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&craneRitem->TexTransform, XMMatrixScaling(0.5, 1.5, 0.35));
	craneRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	craneRitem->Mat = mMaterials["wirefence"].get();
	craneRitem->Geo = mGeometries["buildingsGeo"].get();
	craneRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	craneRitem->IndexCount = craneRitem->Geo->DrawArgs["buildingBox"].IndexCount;
	craneRitem->StartIndexLocation = craneRitem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
	craneRitem->BaseVertexLocation = craneRitem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(craneRitem.get());
	mAllRitems.push_back(std::move(craneRitem));
	//crane horizontal portion
	auto crane2Ritem = std::make_unique<RenderItem>();		// crane length																	
	XMStoreFloat4x4(&crane2Ritem->World, XMMatrixScaling(0.35 * scaleFactor, 2.4 * scaleFactor, 0.35 * scaleFactor) * XMMatrixRotationZ(XMConvertToRadians(90)) * XMMatrixTranslation((8.2 + 0.35 / 2) * scaleFactor,( 3.7 + 0.35 / 2.0f)  * scaleFactor, -4 * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&crane2Ritem->TexTransform, XMMatrixScaling(0.5, 1.5, 0.35));
	crane2Ritem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	crane2Ritem->Mat = mMaterials["wirefence"].get();
	crane2Ritem->Geo = mGeometries["buildingsGeo"].get();
	crane2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	crane2Ritem->IndexCount = crane2Ritem->Geo->DrawArgs["buildingBox"].IndexCount;
	crane2Ritem->StartIndexLocation = crane2Ritem->Geo->DrawArgs["buildingBox"].StartIndexLocation;
	crane2Ritem->BaseVertexLocation = crane2Ritem->Geo->DrawArgs["buildingBox"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(crane2Ritem.get());
	mAllRitems.push_back(std::move(crane2Ritem));
	//crane cable
	auto craneCableRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&craneCableRitem->World, XMMatrixScaling(0.07 * scaleFactor, 3 * scaleFactor, 0.07 * scaleFactor) * XMMatrixTranslation((9 - 1.8 + 0.35 / 2) * scaleFactor, (3.7 - 3 / 2.0f) * scaleFactor, -4 * scaleFactor));/// can choose your scaling here
	craneCableRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	craneCableRitem->Mat = mMaterials["concrete"].get();
	craneCableRitem->Geo = mGeometries["CNTowerGeo"].get();
	craneCableRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	craneCableRitem->IndexCount = craneCableRitem->Geo->DrawArgs["towerPlatformCylinder"].IndexCount;
	craneCableRitem->StartIndexLocation = craneCableRitem->Geo->DrawArgs["towerPlatformCylinder"].StartIndexLocation;
	craneCableRitem->BaseVertexLocation = craneCableRitem->Geo->DrawArgs["towerPlatformCylinder"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(craneCableRitem.get());
	mAllRitems.push_back(std::move(craneCableRitem));
	//diamond crane package
	auto cranePackageRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&cranePackageRitem->World, XMMatrixScaling(0.5 * scaleFactor, 0.5 * scaleFactor, 0.5 * scaleFactor) * XMMatrixTranslation((9 - 1.8 + 0.35 / 2) * scaleFactor, 0.7 * scaleFactor, -4 * scaleFactor));/// can choose your scaling here
	cranePackageRitem->ObjCBIndex = objCBIndex++; /// have to change the object index for it to work
	cranePackageRitem->Mat = mMaterials["brick"].get();
	cranePackageRitem->Geo = mGeometries["diamondGeo"].get();
	cranePackageRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	cranePackageRitem->IndexCount = cranePackageRitem->Geo->DrawArgs["diamond"].IndexCount;
	cranePackageRitem->StartIndexLocation = cranePackageRitem->Geo->DrawArgs["diamond"].StartIndexLocation;
	cranePackageRitem->BaseVertexLocation = cranePackageRitem->Geo->DrawArgs["diamond"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(cranePackageRitem.get());
	mAllRitems.push_back(std::move(cranePackageRitem));


	BoundingBox ConstructionBounds;
	XMFLOAT3 ConstructionCenterf3(9 * scaleFactor, 1.5 / 2.0f * scaleFactor, -4 * scaleFactor);
	XMVECTOR ConstructionCenter = XMLoadFloat3(&ConstructionCenterf3);
	XMFLOAT3 ConstructionExtentsf3(1.5 * scaleFactor /2, 1.5 * scaleFactor /2, 1.2 * scaleFactor/2);
	XMVECTOR ConstructionExtents = XMLoadFloat3(&ConstructionExtentsf3);
	XMStoreFloat3(&ConstructionBounds.Center, ConstructionCenter);
	XMStoreFloat3(&ConstructionBounds.Extents, ConstructionExtents);
	mBoundingBoxes.push_back(ConstructionBounds);


	float road_width = 1;
	
	// intersections
	BuildRenderIntersection(0.75	, 1		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(3.5		, 1		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(4.75	, 1		, road_width, road_width, objCBIndex);
	//BuildRenderIntersection(6.4		, 1		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(7.15	, 1		, road_width, road_width, objCBIndex);

	BuildRenderIntersection(-6.75	, 3		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(-3.75	, 3		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(-1.75	, 3		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(0.75	, 3		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(4.75	, 3		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(7.15	, 3		, road_width, road_width, objCBIndex);

	BuildRenderIntersection(-6.75	, 6		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(-3.75	, 6		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(0.75	, 6		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(4.75	, 6		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(7.15	, 6		, road_width, road_width, objCBIndex);

	BuildRenderIntersection(-6.75	, 9		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(-3.75	, 9		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(0.75	, 9		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(4.75	, 9		, road_width, road_width, objCBIndex);
	BuildRenderIntersection(7.15	, 9		, road_width, road_width, objCBIndex);

	BuildRenderIntersection(-6.75	, 12	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(-3.75	, 12	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(0.75	, 12	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(4.75	, 12	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(7.15	, 12	, road_width, road_width, objCBIndex);

	BuildRenderIntersection(-6.75	, 15	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(-3.75	, 15	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(0.75	, 15	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(4.75	, 15	, road_width, road_width, objCBIndex);
	BuildRenderIntersection(7.15	, 15	, road_width, road_width, objCBIndex);
	road_width = 0.75;
	float z = 1;
	float x = 3.5;
	float x1 = 4.75;
	// roads rows
	//BuildRenderRoad(	2	, -3, 2, road_width, 90.0f, objCBIndex);
	//BuildRenderRoad(	3	, -3, 2, road_width, 0.0f, objCBIndex);
	x = 0.75;
	x1 = 3.5;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 3.5;
	x1 = 4.75;
	BuildRenderRoad((x + x1) / 2	, z	, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	/*x = 4.75;
	x1 = 6.4;
	BuildRenderRoad((x + x1) / 2	, z	, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	*/
	x = 4.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2	, z	, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 10.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	// new row
	z = 3;
	x = -6.75;
	x1 = -10.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -6.75;
	x1 = -3.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -3.75;
	x1 = -1.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 0.75;
	x1 = -1.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 0.75;
	x1 = 4.75;
	BuildRenderRoad((x + x1) / 2	, z	, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 4.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 10.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);

	// new row
	z = 6;
	x = -6.75;
	x1 = -10.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -6.75;
	x1 = -3.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -3.75;
	x1=  0.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 0.75;
	x1 = 4.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 4.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 10.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);

	// new row
	z = 9;
	x = -6.75;
	x1 = -10.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -6.75;
	x1 = -3.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -3.75;
	x1 = 0.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 0.75;
	x1 = 4.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 4.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 10.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);

	// new row
	z = 12;
	x = -6.75;
	x1 = -10.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -6.75;
	x1 = -3.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -3.75;
	x1 = 0.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 0.75;
	x1 = 4.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 4.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 10.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);

	// new row
	z = 15;
	x = -6.75;
	x1 = -10.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -6.75;
	x1 = -3.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = -3.75;
	x1 = 0.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 0.75;
	x1 = 4.75;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 4.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);
	x = 10.75;
	x1 = 7.15;
	BuildRenderRoad((x + x1) / 2, z, fabs(x - x1) - road_width, road_width, 90.0f, objCBIndex);

	//road column
	// new column
	z = 7.15;
	x = 1;
	x1 = 3;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 3;
	x1 = 6;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 6;
	x1 = 9;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 9;
	x1 = 12;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 12;
	x1 = 15;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	// new column
	z = 4.75;
	x = 1;
	x1 = 3;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 3;
	x1 = 6;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 6;
	x1 = 9;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 9;
	x1 = 12;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 12;
	x1 = 15;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	// new column
	z = 0.75;
	x = 1;
	x1 = 3;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 3;
	x1 = 6;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 6;
	x1 = 9;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 9;
	x1 = 12;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 12;
	x1 = 15;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	// new column
	z = -1.75;
	x = 3;
	x1 = -1;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	// new column
	z = -3.75;
	x = 3;
	x1 = 6;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 6;
	x1 = 9;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 9;
	x1 = 12;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 12;
	x1 = 15;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	// new column
	z = -6.75;
	x = 3;
	x1 = 6;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 6;
	x1 = 9;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 9;
	x1 = 12;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);
	x = 12;
	x1 = 15;
	BuildRenderRoad(z, (x + x1) / 2, fabs(x - x1) - road_width, road_width, 0.0f, objCBIndex);

	// trees
	auto treeSpritesRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&treeSpritesRitem->World, XMMatrixScaling(1.0 * scaleFactor, 1.0 * scaleFactor, 1 * scaleFactor));/// can choose your scaling here
	treeSpritesRitem->ObjCBIndex = objCBIndex++;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());
	mAllRitems.push_back(std::move(treeSpritesRitem));

}

void Game::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		//
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Game::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

float Game::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 Game::GetHillsNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}
