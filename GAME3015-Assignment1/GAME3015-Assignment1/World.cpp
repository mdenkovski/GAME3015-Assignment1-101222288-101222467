#include "World.h"

World::World()
{
	mPlane = new Aircraft(Aircraft::Raptor);
	leftPlane = new Aircraft(Aircraft::Eagle);
	rightPlane = new Aircraft(Aircraft::Eagle);
}


void World::update(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList)
{
	if (XMVectorGetX(mPlane->mPosition) > 1.8 || XMVectorGetX(mPlane->mPosition) < -1.8)
	{
		mPlane->mVelocity *= -1;
		leftPlane->mVelocity *= -1;
		rightPlane->mVelocity *= -1;
	}

	mPlane->update(dt, renderList);
	//mPlane->Update();

	leftPlane->update(dt, renderList);
	//leftPlane->Update();

	rightPlane->update(dt, renderList);
	//rightPlane->Update();


	background.update(dt, renderList);
	

	// Apply movements
	mSceneGraph.update(dt, renderList);
}



void World::draw()
{
}

void World::loadTextures(std::unordered_map<std::string, std::unique_ptr<Texture>>& Textures, Microsoft::WRL::ComPtr<ID3D12Device> Device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList)
{
	auto backgroundTex = std::make_unique<Texture>();
	backgroundTex->Name = "BackgroundTex";
	backgroundTex->Filename = L"../../Textures/Desert.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(Device.Get(),
		mCommandList.Get(), backgroundTex->Filename.c_str(),
		backgroundTex->Resource, backgroundTex->UploadHeap));
	Textures[backgroundTex->Name] = std::move(backgroundTex);

	auto EagleTex = std::make_unique<Texture>();
	EagleTex->Name = "EagleTex";
	EagleTex->Filename = L"../../Textures/Eagle.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(Device.Get(),
		mCommandList.Get(), EagleTex->Filename.c_str(),
		EagleTex->Resource, EagleTex->UploadHeap));
	Textures[EagleTex->Name] = std::move(EagleTex);

	auto RaptorTex = std::make_unique<Texture>();
	RaptorTex->Name = "RaptorTex";
	RaptorTex->Filename = L"../../Textures/Raptor.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(Device.Get(),
		mCommandList.Get(), RaptorTex->Filename.c_str(),
		RaptorTex->Resource, RaptorTex->UploadHeap));
	Textures[RaptorTex->Name] = std::move(RaptorTex);
}

void World::buildMaterials(std::unordered_map<std::string, std::unique_ptr<Material>>& Materials)
{
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



	Materials["BackgroundTex"] = std::move(BackgroundTex);
	Materials["EagleTex"] = std::move(Eagle);
	Materials["RaptorTex"] = std::move(Raptor);
}

void World::buildScene(std::vector<std::unique_ptr<RenderItem>>& renderList, std::unordered_map<std::string, std::unique_ptr<Material>>& Materials,
	std::unordered_map<std::string, std::unique_ptr<Texture>>& Textures,
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& Geometries,
	std::vector<RenderItem*> RitemLayer[])
{
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
	background.renderItem->Mat = Materials["BackgroundTex"].get();
	background.renderItem->Geo = Geometries["groundGeo"].get();
	background.renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	background.renderItem->IndexCount = background.renderItem->Geo->DrawArgs["ground"].IndexCount;
	background.renderItem->StartIndexLocation = background.renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	background.renderItem->BaseVertexLocation = background.renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	RitemLayer[(int)RenderLayer::Opaque].push_back(background.renderItem.get());
	renderList.push_back(std::move(background.renderItem));
	background.renderIndex = renderList.size() - 1;
	
	//make the main plane
	mPlane->renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&mPlane->renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(-1.0f, 1, -1));/// can choose your scaling here
	XMVECTOR spawnpoint = { -1.0f , 1 , -1 };
	mPlane->mPosition = spawnpoint;
	mPlane->mVelocity = { 0.5f, 0.0f, 0.0f };
	mPlane->Scale = { 0.01f, 0.01f, 0.01f };
	//XMStorevector(&mPlane->position, spawnpoint);
	XMStoreFloat4x4(&mPlane->renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	mPlane->renderItem->ObjCBIndex = objCBIndex++;
	mPlane->renderItem->Mat = Materials["RaptorTex"].get();
	mPlane->renderItem->Geo = Geometries["groundGeo"].get();
	mPlane->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mPlane->renderItem->IndexCount = mPlane->renderItem->Geo->DrawArgs["ground"].IndexCount;
	mPlane->renderItem->StartIndexLocation = mPlane->renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	mPlane->renderItem->BaseVertexLocation = mPlane->renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	RitemLayer[(int)RenderLayer::AlphaTested].push_back(mPlane->renderItem.get());
	renderList.push_back(std::move(mPlane->renderItem));
	mPlane->renderIndex = renderList.size() - 1;

	//make the left plane
	leftPlane->renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&leftPlane->renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(-1.25f, 1, -1.25));/// can choose your scaling here
	spawnpoint = { -1.25f , 1.0f , -1.25f };
	leftPlane->mPosition = spawnpoint;
	leftPlane->mVelocity = { 0.5f, 0.0f, 0.0f };
	leftPlane->Scale = { 0.01f, 0.01f, 0.01f };
	XMStoreFloat4x4(&leftPlane->renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	leftPlane->renderItem->ObjCBIndex = objCBIndex++;
	leftPlane->renderItem->Mat = Materials["EagleTex"].get();
	leftPlane->renderItem->Geo = Geometries["groundGeo"].get();
	leftPlane->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftPlane->renderItem->IndexCount = leftPlane->renderItem->Geo->DrawArgs["ground"].IndexCount;
	leftPlane->renderItem->StartIndexLocation = leftPlane->renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	leftPlane->renderItem->BaseVertexLocation = leftPlane->renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	RitemLayer[(int)RenderLayer::AlphaTested].push_back(leftPlane->renderItem.get());
	renderList.push_back(std::move(leftPlane->renderItem));
	leftPlane->renderIndex = renderList.size() - 1;


	//make the right plane
	rightPlane->renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&rightPlane->renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(-0.75f, 1, -1.25));/// can choose your scaling here
	spawnpoint = { -0.75f , 1.0f , -1.25f };
	rightPlane->mPosition = spawnpoint;
	rightPlane->mVelocity = { 0.5f, 0.0f, 0.0f };
	rightPlane->Scale = { 0.01f, 0.01f, 0.01f };
	XMStoreFloat4x4(&rightPlane->renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	rightPlane->renderItem->ObjCBIndex = objCBIndex++;
	rightPlane->renderItem->Mat = Materials["EagleTex"].get();
	rightPlane->renderItem->Geo = Geometries["groundGeo"].get();
	rightPlane->renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightPlane->renderItem->IndexCount = rightPlane->renderItem->Geo->DrawArgs["ground"].IndexCount;
	rightPlane->renderItem->StartIndexLocation = rightPlane->renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	rightPlane->renderItem->BaseVertexLocation = rightPlane->renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	RitemLayer[(int)RenderLayer::AlphaTested].push_back(rightPlane->renderItem.get());
	renderList.push_back(std::move(rightPlane->renderItem));
	rightPlane->renderIndex = renderList.size() - 1;



}