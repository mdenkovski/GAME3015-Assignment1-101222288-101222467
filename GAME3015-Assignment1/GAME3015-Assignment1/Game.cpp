//***************************************************************************************
// Game.cpp 
//***************************************************************************************
#include "Game.h"



	const int gNumFrameResources = 3;

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
	mCamera.SetPosition(0, 5, 0 );
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

	UpdateGameObjects(gt);
	//GameWorld.update(gt, mAllRitems);

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
	float buffer = 0.5 ;
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
	mMainPassCB.Lights[3].Position = { -5.0f  , 3.0f , 0.0f  };
	mMainPassCB.Lights[3].Strength = { 1.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[3].Direction = { 0.0, -1.0, 0.0f };
	mMainPassCB.Lights[3].FalloffStart = { 1.0f   };
	mMainPassCB.Lights[3].FalloffEnd = { 5.0f  };
	mMainPassCB.Lights[3].SpotPower = { 1.0f };


	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void Game::MoveGameObjects(const GameTimer& gt)
{
	//GameWorld.update(gt, mAllRitems);

	////check if plane is at edge
	//if (XMVectorGetX(plane.position) > 1.8 || XMVectorGetX(plane.position) < -1.8 )
	//{
	//	plane.velocity *= -1;
	//	leftPlane.velocity *= -1;
	//	rightPlane.velocity *= -1;
	//}

	

	////move the main plane
	//XMVECTOR displacement = plane.velocity * gt.DeltaTime();
	//plane.position = XMVectorAdd(plane.position, displacement);
	//plane.renderItem = std::move(mAllRitems[plane.renderIndex]);
	//plane.renderItem->NumFramesDirty = 1;
	//XMStoreFloat4x4(&plane.renderItem->World, XMMatrixScaling(0.01f , 0.01f , 0.01f ) * XMMatrixTranslationFromVector(plane.position));
	//mAllRitems[plane.renderIndex] = std::move(plane.renderItem);

	////move the supporting planes
	//displacement = leftPlane.velocity * gt.DeltaTime(); 
	//leftPlane.position = XMVectorAdd(leftPlane.position, displacement);
	//leftPlane.renderItem = std::move(mAllRitems[leftPlane.renderIndex]);
	//leftPlane.renderItem->NumFramesDirty = 1;
	//XMStoreFloat4x4(&leftPlane.renderItem->World, XMMatrixScaling(0.01f, 0.01f , 0.01f ) * XMMatrixTranslationFromVector(leftPlane.position));
	//mAllRitems[leftPlane.renderIndex] = std::move(leftPlane.renderItem);

	//displacement = rightPlane.velocity * gt.DeltaTime();
	//rightPlane.position = XMVectorAdd(rightPlane.position, displacement);
	//rightPlane.renderItem = std::move(mAllRitems[rightPlane.renderIndex]);
	//rightPlane.renderItem->NumFramesDirty = 1;
	//XMStoreFloat4x4(&rightPlane.renderItem->World, XMMatrixScaling(0.01f , 0.01f, 0.01f) * XMMatrixTranslationFromVector(rightPlane.position));
	//mAllRitems[rightPlane.renderIndex] = std::move(rightPlane.renderItem);

	////move the background down
	//displacement = background.velocity * gt.DeltaTime();
	//background.position = XMVectorAdd(background.position, displacement);
	//background.renderItem = std::move(mAllRitems[background.renderIndex]);
	//background.renderItem->NumFramesDirty = 1;
	//XMStoreFloat4x4(&background.renderItem->World, XMMatrixScaling(1.0f , 1.0f , 1.0f ) * XMMatrixTranslationFromVector(background.position));
	//mAllRitems[background.renderIndex] = std::move(background.renderItem);

	
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

void Game::UpdateGameObjects(const GameTimer& gt)
{
	if (XMVectorGetX(mPlane->mPosition) > 1.8 || XMVectorGetX(mPlane->mPosition) < -1.8)
	{
		mPlane->mVelocity *= -1;
		leftPlane->mVelocity *= -1;
		rightPlane->mVelocity *= -1;
	}

	if (XMVectorGetZ(background.mPosition) < -12)
	{
		background.mPosition = { XMVectorGetX(background.mPosition) , XMVectorGetY(background.mPosition) , 12 };
	}

	if (XMVectorGetZ(background2.mPosition) < -12)
	{
		background2.mPosition = { XMVectorGetX(background2.mPosition) , XMVectorGetY(background2.mPosition) , 12 };
	}

	mPlane->update(gt, mAllRitems);
	//mPlane->Update();

	leftPlane->update(gt, mAllRitems);
	//leftPlane->Update();

	rightPlane->update(gt, mAllRitems);
	//rightPlane->Update();


	background.update(gt, mAllRitems);
	background2.update(gt, mAllRitems);


	// Apply movements
	mSceneGraph.update(gt, mAllRitems);
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
	//GameWorld.buildMaterials(mMaterials);

	int matIndex = 0;
	auto BackgroundTex = std::make_unique<Material>();
	BackgroundTex->Name = "BackgroundTex";
	BackgroundTex->MatCBIndex = matIndex;
	BackgroundTex->DiffuseSrvHeapIndex = matIndex++;
	BackgroundTex->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	BackgroundTex->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	BackgroundTex->Roughness = 0.125f;

	

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

	

	mMaterials["BackgroundTex"] = std::move(BackgroundTex);
	mMaterials["EagleTex"] = std::move(Eagle);
	mMaterials["RaptorTex"] = std::move(Raptor);
	

}


void Game::BuildRenderItems()
{
	//GameWorld.buildScene(mAllRitems, mMaterials, mTextures, mGeometries, mRitemLayer);


	UINT objCBIndex = 0;

	// Initialize the different layers
	for (std::size_t i = 0; i < LayerCount; ++i)
	{
		SceneNode::Ptr layer(new SceneNode());
		mSceneLayers[i] = layer.get();

		mSceneGraph.attachChild(std::move(layer));
	}



	background.renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&background.renderItem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0, 0));/// can choose your scaling here
	XMStoreFloat4x4(&background.renderItem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0f));
	XMVECTOR spawnpointBackground = { 0, 0, 0 };
	background.mPosition = spawnpointBackground;
	background.mVelocity = { 0.0f, 0.0f, -0.5f };
	background.Scale = { 1.0f, 1.0f, 1.0f };
	background.renderItem->ObjCBIndex = objCBIndex++;
	background.renderItem->Mat = mMaterials["BackgroundTex"].get();
	background.renderItem->Geo = mGeometries["groundGeo"].get();
	background.renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	background.renderItem->IndexCount = background.renderItem->Geo->DrawArgs["ground"].IndexCount;
	background.renderItem->StartIndexLocation = background.renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	background.renderItem->BaseVertexLocation = background.renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(background.renderItem.get());
	mAllRitems.push_back(std::move(background.renderItem));
	background.renderIndex = mAllRitems.size() - 1;

	background2.renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&background2.renderItem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0, 12));/// can choose your scaling here
	XMStoreFloat4x4(&background2.renderItem->TexTransform, XMMatrixScaling(10.0f, 10.0f, 10.0f));
	XMVECTOR spawnpointbackground2 = { 0, 0, 12 };
	background2.mPosition = spawnpointbackground2;
	background2.mVelocity = { 0.0f, 0.0f, -0.5f };
	background2.Scale = { 1.0f, 1.0f, 1.0f };
	background2.renderItem->ObjCBIndex = objCBIndex++;
	background2.renderItem->Mat = mMaterials["BackgroundTex"].get();
	background2.renderItem->Geo = mGeometries["groundGeo"].get();
	background2.renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	background2.renderItem->IndexCount = background2.renderItem->Geo->DrawArgs["ground"].IndexCount;
	background2.renderItem->StartIndexLocation = background2.renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	background2.renderItem->BaseVertexLocation = background2.renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(background2.renderItem.get());
	mAllRitems.push_back(std::move(background2.renderItem));
	background2.renderIndex = mAllRitems.size() - 1;




	//make the main plane
	mPlane = new Aircraft(Aircraft::Raptor);
	mPlane->renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&mPlane->renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(-1.0f, 1, -1));/// can choose your scaling here
	XMVECTOR spawnpoint = { -1.0f , 1 , -1 };
	mPlane->mPosition = spawnpoint;
	mPlane->mVelocity = { 0.5f, 0.0f, 0.0f };
	mPlane->Scale = { 0.01f, 0.01f, 0.01f };
	//XMStorevector(&mPlane->position, spawnpoint);
	XMStoreFloat4x4(&mPlane->renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	mPlane->renderItem->ObjCBIndex = objCBIndex++;
	mPlane->renderItem->Mat = mMaterials["RaptorTex"].get();
	mPlane->renderItem->Geo = mGeometries["groundGeo"].get();
	mPlane->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mPlane->renderItem->IndexCount = mPlane->renderItem->Geo->DrawArgs["ground"].IndexCount;
	mPlane->renderItem->StartIndexLocation = mPlane->renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	mPlane->renderItem->BaseVertexLocation = mPlane->renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(mPlane->renderItem.get());
	mAllRitems.push_back(std::move(mPlane->renderItem));
	mPlane->renderIndex = mAllRitems.size() - 1;

	//make the left plane
	leftPlane = new Aircraft(Aircraft::Eagle);
	leftPlane->renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&leftPlane->renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(-1.25f, 1, -1.25));/// can choose your scaling here
	spawnpoint = { -1.25f , 1.0f , -1.25f };
	leftPlane->mPosition = spawnpoint;
	leftPlane->mVelocity = { 0.5f, 0.0f, 0.0f };
	leftPlane->Scale = { 0.01f, 0.01f, 0.01f };
	XMStoreFloat4x4(&leftPlane->renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	leftPlane->renderItem->ObjCBIndex = objCBIndex++;
	leftPlane->renderItem->Mat = mMaterials["EagleTex"].get();
	leftPlane->renderItem->Geo = mGeometries["groundGeo"].get();
	leftPlane->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftPlane->renderItem->IndexCount = leftPlane->renderItem->Geo->DrawArgs["ground"].IndexCount;
	leftPlane->renderItem->StartIndexLocation = leftPlane->renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	leftPlane->renderItem->BaseVertexLocation = leftPlane->renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(leftPlane->renderItem.get());
	mAllRitems.push_back(std::move(leftPlane->renderItem));
	leftPlane->renderIndex = mAllRitems.size() - 1;


	//make the right plane
	rightPlane = new Aircraft(Aircraft::Eagle);
	rightPlane->renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&rightPlane->renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(-0.75f, 1, -1.25));/// can choose your scaling here
	spawnpoint = { -0.75f , 1.0f , -1.25f };
	rightPlane->mPosition = spawnpoint;
	rightPlane->mVelocity = { 0.5f, 0.0f, 0.0f };
	rightPlane->Scale = { 0.01f, 0.01f, 0.01f };
	XMStoreFloat4x4(&rightPlane->renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	rightPlane->renderItem->ObjCBIndex = objCBIndex++;
	rightPlane->renderItem->Mat = mMaterials["EagleTex"].get();
	rightPlane->renderItem->Geo = mGeometries["groundGeo"].get();
	rightPlane->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightPlane->renderItem->IndexCount = rightPlane->renderItem->Geo->DrawArgs["ground"].IndexCount;
	rightPlane->renderItem->StartIndexLocation = rightPlane->renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	rightPlane->renderItem->BaseVertexLocation = rightPlane->renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(rightPlane->renderItem.get());
	mAllRitems.push_back(std::move(rightPlane->renderItem));
	rightPlane->renderIndex = mAllRitems.size() - 1;

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




