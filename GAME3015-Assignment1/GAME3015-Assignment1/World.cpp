#include "World.h"

World::World()
{
}


void World::update(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList)
{
	mPlane.update(dt, renderList);
	mPlane.Update();


	background.update(dt, renderList);
	//// Scroll the world
	//mWorldView.move(0.f, mScrollSpeed * dt.asSeconds());

	//// Move the player sidewards (plane scouts follow the main aircraft)
	//sf::Vector2f position = mPlayerAircraft->getPosition();
	//sf::Vector2f velocity = mPlayerAircraft->getVelocity();

	//// If player touches borders, flip its X velocity
	//if (position.x <= mWorldBounds.left + 150.f
	//	|| position.x >= mWorldBounds.left + mWorldBounds.width - 150.f)
	//{
	//	velocity.x = -velocity.x;
	//	mPlayerAircraft->setVelocity(velocity);
	//}

	// Apply movements
	mSceneGraph.update(dt, renderList);
}



void World::draw()
{
}

void World::loadTextures()
{

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
	
	mPlane.renderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&mPlane.renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(0.0f, 1, 0.0f));/// can choose your scaling here
	XMVECTOR spawnpoint = { 0.0f, 1.0f, 0.0f };
	mPlane.mPosition = spawnpoint;
	mPlane.mVelocity = { 0.5f, 0.0f, 0.0f };
	mPlane.Scale = { 0.01f, 0.01f, 0.01f };
	//XMStorevector(&mPlane.position, spawnpoint);
	XMStoreFloat4x4(&mPlane.renderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	mPlane.renderItem->ObjCBIndex = objCBIndex++;
	mPlane.renderItem->Mat = Materials["RaptorTex"].get();
	mPlane.renderItem->Geo = Geometries["groundGeo"].get();
	mPlane.renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mPlane.renderItem->IndexCount = mPlane.renderItem->Geo->DrawArgs["ground"].IndexCount;
	mPlane.renderItem->StartIndexLocation = mPlane.renderItem->Geo->DrawArgs["ground"].StartIndexLocation;
	mPlane.renderItem->BaseVertexLocation = mPlane.renderItem->Geo->DrawArgs["ground"].BaseVertexLocation;

	RitemLayer[(int)RenderLayer::AlphaTested].push_back(mPlane.renderItem.get());
	renderList.push_back(std::move(mPlane.renderItem));
	mPlane.renderIndex = renderList.size() - 1;

	//// Add the background sprite to the scene
	//std::unique_ptr<SceneNode> backgroundSprite(new SceneNode());
	//backgroundSprite->setPosition(mWorldBounds.left, mWorldBounds.top);
	//mSceneLayers[Background]->attachChild(std::move(backgroundSprite));

	//// Add player's aircraft
	//std::unique_ptr<Aircraft> leader(new Aircraft(Aircraft::Eagle, mTextures));
	//mPlayerAircraft = leader.get();
	//mPlayerAircraft->setPosition(mSpawnPosition);
	//mPlayerAircraft->setVelocity(40.f, mScrollSpeed);
	//mSceneLayers[Air]->attachChild(std::move(leader));

	//// Add two escorting aircrafts, placed relatively to the main plane
	//std::unique_ptr<Aircraft> leftEscort(new Aircraft(Aircraft::Raptor, mTextures));
	//leftEscort->setPosition(-80.f, 50.f);
	//mPlayerAircraft->attachChild(std::move(leftEscort));

	//std::unique_ptr<Aircraft> rightEscort(new Aircraft(Aircraft::Raptor, mTextures));
	//rightEscort->setPosition(80.f, 50.f);
	//mPlayerAircraft->attachChild(std::move(rightEscort));
}