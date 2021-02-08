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
#include "SceneNode.h"
#include <ctime>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;



//// Lightweight structure stores parameters to draw a shape.  This will
//// vary from app-to-app.
//struct RenderItem
//{
//	RenderItem() = default;
//
//	// World matrix of the shape that describes the object's local space
//	// relative to the world space, which defines the position, orientation,
//	// and scale of the object in the world.
//	XMFLOAT4X4 World = MathHelper::Identity4x4();
//
//	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
//
//	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
//	// Because we have an object cbuffer for each FrameResource, we have to apply the
//	// update to each FrameResource.  Thus, when we modify obect data we should set 
//	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
//	int NumFramesDirty = gNumFrameResources;
//
//	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
//	UINT ObjCBIndex = -1;
//
//	Material* Mat = nullptr;
//	MeshGeometry* Geo = nullptr;
//
//	// Primitive topology.
//	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//
//	// DrawIndexedInstanced parameters.
//	UINT IndexCount = 0;
//	UINT StartIndexLocation = 0;
//	int BaseVertexLocation = 0;
//};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

struct GameObject
{
	XMVECTOR position;
	XMVECTOR velocity;
	std::unique_ptr<RenderItem> renderItem;
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

	void MoveGameObjects(const GameTimer& gt);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayouts();

	//void BuildLandGeometry();

	//void BuildCityLandGeometry();
	void BuildGroundGeometry();
	//void BuildCNTowerGeometry();
	//void BuildRogersCenter();
	//void BuildBuildings();
	//void BuildWavesGeometry();
	//void BuildBoxGeometry();
	//void BuildDiamondGeometry();
	//void BuildTreeSpritesGeometry();


	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	//void BuildRenderPyra(float x, float y, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex);
	//void BuildRenderFlatTopPyra(float x, float y, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex);
	//void BuildRenderBuilding(float x, float z, float scalex, float scaley, float scalez, UINT& ObjCBIndex);
	//void BuildRenderRoad(float x, float z, float length,float width, float angle, UINT& ObjCBIndex);
	//void BuildRenderIntersection(float x, float z, float scalex, float scalez, UINT& ObjCBIndex);
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

	GameObject plane;
	SceneNode mSceneGraph;
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

	//mCamera.SetPosition(0.8f * scaleFactor, 0.3 * scaleFactor, 1.0f * scaleFactor);
	mCamera.SetPosition(0 * scaleFactor, 5 * scaleFactor, 0 * scaleFactor);
	mCamera.Pitch(3.14/2);

	mWaves = std::make_unique<Waves>(40, 80, 0.25, 0.03f, 1.0, 0.2f);

	//Step 1 Load the textures
	LoadTextures();

	BuildRootSignature();

	//Step 2 Build Descriptor heaps to allow for the new texture
	BuildDescriptorHeaps();
	BuildShadersAndInputLayouts();

	// Step 3 Build the geometry for your shapes
	BuildGroundGeometry();

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

	//AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);

	MoveGameObjects(gt);
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

	/*mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);*/

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
	//mCamera.SetPositionY(0.3 * scaleFactor);
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

//void Game::AnimateMaterials(const GameTimer& gt)
//{
//	// Scroll the water material texture coordinates.
//	auto waterMat = mMaterials["water"].get();
//
//	float& tu = waterMat->MatTransform(3, 0);
//	float& tv = waterMat->MatTransform(3, 1);
//
//	tu += 0.1f * gt.DeltaTime();
//	tv += 0.02f * gt.DeltaTime();
//
//	if (tu >= 1.0f)
//		tu -= 1.0f;
//
//	if (tv >= 1.0f)
//		tv -= 1.0f;
//
//	waterMat->MatTransform(3, 0) = tu;
//	waterMat->MatTransform(3, 1) = tv;
//
//	// Material has changed, so need to update cbuffer.
//	waterMat->NumFramesDirty = gNumFrameResources;
//}

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

void Game::MoveGameObjects(const GameTimer& gt)
{
	XMVECTOR displacement = plane.velocity * gt.DeltaTime();
	plane.position = XMVectorAdd(plane.position, displacement);
	plane.renderItem = std::move(mAllRitems[1]);
	plane.renderItem->NumFramesDirty = 1;
	XMStoreFloat4x4(&plane.renderItem->World, XMMatrixScaling(0.01f * scaleFactor, 0.01f * scaleFactor, 0.01f * scaleFactor) * XMMatrixTranslationFromVector(plane.position));
	mAllRitems[1] = std::move(plane.renderItem);


	//auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	//for (auto& e : mAllRitems)
	//{
	//	// Only update the cbuffer data if the constants have changed.  
	//	// This needs to be tracked per frame resource.
	//	if (e->NumFramesDirty > 0)
	//	{
	//		XMMATRIX world = XMLoadFloat4x4(&e->World);
	//		XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

	//		ObjectConstants objConstants;
	//		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
	//		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

	//		currObjectCB->CopyData(e->ObjCBIndex, objConstants);

	//		// Next FrameResource need to be updated too.
	//		e->NumFramesDirty--;
	//	}
	//}

	/*mAllRitems.
	mAllRitems.push_back(std::move(plane.renderItem));*/
}

void Game::LoadTextures()
{
	auto backgroundTex = std::make_unique<Texture>();
	backgroundTex->Name = "BackgroundTex";
	backgroundTex->Filename = L"../../Textures/Desert.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), backgroundTex->Filename.c_str(),
		backgroundTex->Resource, backgroundTex->UploadHeap));
	mTextures[backgroundTex->Name] = std::move(backgroundTex);

	auto EagleTex = std::make_unique<Texture>();
	EagleTex->Name = "EagleTex";
	EagleTex->Filename = L"../../Textures/Eagle.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), EagleTex->Filename.c_str(),
		EagleTex->Resource, EagleTex->UploadHeap));
	mTextures[EagleTex->Name] = std::move(EagleTex);

	auto RaptorTex = std::make_unique<Texture>();
	RaptorTex->Name = "RaptorTex";
	RaptorTex->Filename = L"../../Textures/Raptor.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), RaptorTex->Filename.c_str(),
		RaptorTex->Resource, RaptorTex->UploadHeap));
	mTextures[RaptorTex->Name] = std::move(RaptorTex);

	//auto waterTex = std::make_unique<Texture>();
	//waterTex->Name = "waterTex";
	//waterTex->Filename = L"../../Textures/water1.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), waterTex->Filename.c_str(),
	//	waterTex->Resource, waterTex->UploadHeap));
	//mTextures[waterTex->Name] = std::move(waterTex);

	//auto roofTex = std::make_unique<Texture>();
	//roofTex->Name = "roofTex";
	//roofTex->Filename = L"../../Textures/Roof.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), roofTex->Filename.c_str(),
	//	roofTex->Resource, roofTex->UploadHeap));
	//mTextures[roofTex->Name] = std::move(roofTex);

	//auto buildingTex = std::make_unique<Texture>();
	//buildingTex->Name = "buildingTex";
	//buildingTex->Filename = L"../../Textures/building.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), buildingTex->Filename.c_str(),
	//	buildingTex->Resource, buildingTex->UploadHeap));
	//mTextures[buildingTex->Name] = std::move(buildingTex);

	//auto concreteTex = std::make_unique<Texture>();
	//concreteTex->Name = "concreteTex";
	//concreteTex->Filename = L"../../Textures/concrete.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), concreteTex->Filename.c_str(),
	//	concreteTex->Resource, concreteTex->UploadHeap));
	//mTextures[concreteTex->Name] = std::move(concreteTex);

	//auto brickTex = std::make_unique<Texture>();
	//brickTex->Name = "brickTex";
	//brickTex->Filename = L"../../Textures/bricks.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), brickTex->Filename.c_str(),
	//	brickTex->Resource, brickTex->UploadHeap));
	//mTextures[brickTex->Name] = std::move(brickTex);

	//auto fenceTex = std::make_unique<Texture>();
	//fenceTex->Name = "fenceTex";
	//fenceTex->Filename = L"../../Textures/WireFence.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), fenceTex->Filename.c_str(),
	//	fenceTex->Resource, fenceTex->UploadHeap));
	//mTextures[fenceTex->Name] = std::move(fenceTex);

	//auto roadTex = std::make_unique<Texture>();
	//roadTex->Name = "roadTex";
	//roadTex->Filename = L"../../Textures/road.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), roadTex->Filename.c_str(),
	//	roadTex->Resource, roadTex->UploadHeap));
	//mTextures[roadTex->Name] = std::move(roadTex);

	//auto roadITex = std::make_unique<Texture>();
	//roadITex->Name = "roadITex";
	//roadITex->Filename = L"../../Textures/intersection.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), roadITex->Filename.c_str(),
	//	roadITex->Resource, roadITex->UploadHeap));
	//mTextures[roadITex->Name] = std::move(roadITex);

	//auto treeArrayTex = std::make_unique<Texture>();
	//treeArrayTex->Name = "treeArrayTex";
	//treeArrayTex->Filename = L"../../Textures/treeArray.dds";
	//ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
	//	mCommandList.Get(), treeArrayTex->Filename.c_str(),
	//	treeArrayTex->Resource, treeArrayTex->UploadHeap));
	//mTextures[treeArrayTex->Name] = std::move(treeArrayTex);


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

	auto backgroundTex = mTextures["BackgroundTex"]->Resource;
	auto EagleTex = mTextures["EagleTex"]->Resource;
	auto RaptorTex = mTextures["RaptorTex"]->Resource;

	//auto waterTex = mTextures["waterTex"]->Resource;
	//auto roofTex = mTextures["roofTex"]->Resource;
	//auto buildingTex = mTextures["buildingTex"]->Resource;
	//auto concreteTex = mTextures["concreteTex"]->Resource;
	//auto brickTex = mTextures["brickTex"]->Resource;
	//auto fenceTex = mTextures["fenceTex"]->Resource;
	//auto roadTex = mTextures["roadTex"]->Resource;
	//auto roadITex = mTextures["roadITex"]->Resource;
	//auto treeArrayTex = mTextures["treeArrayTex"]->Resource;


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = backgroundTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(backgroundTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = EagleTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(EagleTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = RaptorTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(RaptorTex.Get(), &srvDesc, hDescriptor);

	//// next descriptor - building texture
	//hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//srvDesc.Format = buildingTex->GetDesc().Format;
	//md3dDevice->CreateShaderResourceView(buildingTex.Get(), &srvDesc, hDescriptor);

	//// next descriptor - concrete texture
	//hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//srvDesc.Format = concreteTex->GetDesc().Format;
	//md3dDevice->CreateShaderResourceView(concreteTex.Get(), &srvDesc, hDescriptor);

	//// next descriptor - brick texture
	//hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//srvDesc.Format = brickTex->GetDesc().Format;
	//md3dDevice->CreateShaderResourceView(brickTex.Get(), &srvDesc, hDescriptor);

	//// next descriptor - fence texture
	//hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//srvDesc.Format = fenceTex->GetDesc().Format;
	//md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);

	//// next descriptor - road texture
	//hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//srvDesc.Format = roadTex->GetDesc().Format;
	//md3dDevice->CreateShaderResourceView(roadTex.Get(), &srvDesc, hDescriptor);

	//// next descriptor - roadI texture
	//hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//srvDesc.Format = roadITex->GetDesc().Format;
	//md3dDevice->CreateShaderResourceView(roadITex.Get(), &srvDesc, hDescriptor);

	//// next descriptor
	//hDescriptor.Offset(1, mCbvSrvDescriptorSize);


	//auto desc = treeArrayTex->GetDesc();
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	//srvDesc.Format = treeArrayTex->GetDesc().Format;
	//srvDesc.Texture2DArray.MostDetailedMip = 0;
	//srvDesc.Texture2DArray.MipLevels = -1;
	//srvDesc.Texture2DArray.FirstArraySlice = 0;
	//srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	//md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);


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

	////
	//// PSO for tree sprites
	////
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
	auto BackgroundTex = std::make_unique<Material>();
	BackgroundTex->Name = "BackgroundTex";
	BackgroundTex->MatCBIndex = matIndex;
	BackgroundTex->DiffuseSrvHeapIndex = matIndex++;
	BackgroundTex->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	BackgroundTex->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	BackgroundTex->Roughness = 0.125f;

	//// This is not a good water material definition, but we do not have all the rendering
	//// tools we need (transparency, environment reflection), so we fake it for now.
	//auto water = std::make_unique<Material>();
	//water->Name = "water";
	//water->MatCBIndex = matIndex;
	//water->DiffuseSrvHeapIndex = matIndex++;
	//water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	//water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	//water->Roughness = 0.0f;

	auto Eagle = std::make_unique<Material>();
	Eagle->Name = "Eagle";
	Eagle->MatCBIndex = matIndex;
	Eagle->DiffuseSrvHeapIndex = matIndex++;
	Eagle->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Eagle->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	Eagle->Roughness = 0.25f;

	auto Raptor = std::make_unique<Material>();
	Raptor->Name = "Raptor";
	Raptor->MatCBIndex = matIndex;
	Raptor->DiffuseSrvHeapIndex = matIndex++;
	Raptor->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Raptor->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	Raptor->Roughness = 0.125f;

	//auto concrete = std::make_unique<Material>();
	//concrete->Name = "concrete";
	//concrete->MatCBIndex = matIndex;
	//concrete->DiffuseSrvHeapIndex = matIndex++;
	//concrete->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//concrete->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//concrete->Roughness = 0.125f;

	//auto brick = std::make_unique<Material>();
	//brick->Name = "brick";
	//brick->MatCBIndex = matIndex;
	//brick->DiffuseSrvHeapIndex = matIndex++;
	//brick->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//brick->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//brick->Roughness = 0.125f;

	//auto wirefence = std::make_unique<Material>();
	//wirefence->Name = "wirefence";
	//wirefence->MatCBIndex = matIndex;
	//wirefence->DiffuseSrvHeapIndex = matIndex++;
	//wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//wirefence->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	//wirefence->Roughness = 0.25f;

	//auto road = std::make_unique<Material>();
	//road->Name = "road";
	//road->MatCBIndex = matIndex;
	//road->DiffuseSrvHeapIndex = matIndex++;
	//road->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//road->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//road->Roughness = 0.125f;

	//auto roadI = std::make_unique<Material>();
	//roadI->Name = "roadI";
	//roadI->MatCBIndex = matIndex;
	//roadI->DiffuseSrvHeapIndex = matIndex++;
	//roadI->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//roadI->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//roadI->Roughness = 0.125f;


	//auto treeSprites = std::make_unique<Material>();
	//treeSprites->Name = "treeSprites";
	//treeSprites->MatCBIndex = matIndex;
	//treeSprites->DiffuseSrvHeapIndex = matIndex++;
	//treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//treeSprites->Roughness = 0.125f;

	mMaterials["BackgroundTex"] = std::move(BackgroundTex);
	mMaterials["EagleTex"] = std::move(Eagle);
	mMaterials["RaptorTex"] = std::move(Raptor);
	//mMaterials["building"] = std::move(building);
	//mMaterials["concrete"] = std::move(concrete);
	//mMaterials["brick"] = std::move(brick);
	//mMaterials["wirefence"] = std::move(wirefence);
	//mMaterials["road"] = std::move(road);
	//mMaterials["roadI"] = std::move(roadI);
	//mMaterials["treeSprites"] = std::move(treeSprites);

}


void Game::BuildRenderItems()
{
	UINT objCBIndex = 0;

	////the ground under the ocean
	//auto gridRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&gridRitem->World, XMMatrixScaling(1.0f * scaleFactor, 0.02f * scaleFactor, 1.0f * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, -0.2f * scaleFactor, 0.0f * scaleFactor));
	//XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	//gridRitem->ObjCBIndex = objCBIndex++;
	//gridRitem->Mat = mMaterials["grass"].get();
	//gridRitem->Geo = mGeometries["landGeo"].get();
	//gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	//gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	//gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	//mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	//mAllRitems.push_back(std::move(gridRitem));

	// land for the city
	auto groundRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&groundRitem->World, XMMatrixScaling(1.0f * scaleFactor, 1.0f * scaleFactor, 1.0f * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, 0 * scaleFactor, 0 * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&groundRitem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0f));
	groundRitem->ObjCBIndex = objCBIndex++;
	groundRitem->Mat = mMaterials["BackgroundTex"].get();
	groundRitem->Geo = mGeometries["groundGeo"].get();
	groundRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	groundRitem->IndexCount = groundRitem->Geo->DrawArgs["ground"].IndexCount;
	groundRitem->StartIndexLocation = groundRitem->Geo->DrawArgs["ground"].StartIndexLocation;
	groundRitem->BaseVertexLocation = groundRitem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(groundRitem.get());
	mAllRitems.push_back(std::move(groundRitem));


	plane.renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&plane.renderItem->World, XMMatrixScaling(0.01f * scaleFactor, 0.01f * scaleFactor, 0.01f * scaleFactor) * XMMatrixTranslation(-1.0f * scaleFactor, 1 * scaleFactor, -1 * scaleFactor));/// can choose your scaling here
	XMVECTOR spawnpoint = { -1.0f * scaleFactor, 1 * scaleFactor, -1 * scaleFactor };
	plane.position = spawnpoint;
	plane.velocity = {5.0f, 0.0f, 0.0f};
	//XMStorevector(&plane.position, spawnpoint);
	XMStoreFloat4x4(&plane.renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	plane.renderItem->ObjCBIndex = objCBIndex++;
	plane.renderItem->Mat = mMaterials["RaptorTex"].get();
	plane.renderItem->Geo = mGeometries["groundGeo"].get();
	plane.renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	plane.renderItem->IndexCount = plane.renderItem->Geo->DrawArgs["ground"].IndexCount;
	plane.renderItem->StartIndexLocation = plane.renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	plane.renderItem->BaseVertexLocation = plane.renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(plane.renderItem.get());
	mAllRitems.push_back(std::move(plane.renderItem));






	auto raptor = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&raptor->World, XMMatrixScaling(0.01f * scaleFactor, 0.01f * scaleFactor, 0.01f * scaleFactor) * XMMatrixTranslation(0.0f * scaleFactor, 1 * scaleFactor, 0 * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&raptor->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	raptor->ObjCBIndex = objCBIndex++;
	raptor->Mat = mMaterials["RaptorTex"].get();
	raptor->Geo = mGeometries["groundGeo"].get();
	raptor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	raptor->IndexCount = raptor->Geo->DrawArgs["ground"].IndexCount;
	raptor->StartIndexLocation = raptor->Geo->DrawArgs["ground"].StartIndexLocation;
	raptor->BaseVertexLocation = raptor->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(raptor.get());
	mAllRitems.push_back(std::move(raptor));

	auto eagle = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&eagle->World, XMMatrixScaling(0.01f * scaleFactor, 0.01f * scaleFactor, 0.01f * scaleFactor) * XMMatrixTranslation(1.0f * scaleFactor, 1 * scaleFactor, 0 * scaleFactor));/// can choose your scaling here
	XMStoreFloat4x4(&eagle->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	eagle->ObjCBIndex = objCBIndex++;
	eagle->Mat = mMaterials["EagleTex"].get();
	eagle->Geo = mGeometries["groundGeo"].get();
	eagle->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	eagle->IndexCount = eagle->Geo->DrawArgs["ground"].IndexCount;
	eagle->StartIndexLocation = eagle->Geo->DrawArgs["ground"].StartIndexLocation;
	eagle->BaseVertexLocation = eagle->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(eagle.get());
	mAllRitems.push_back(std::move(eagle));
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