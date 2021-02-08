#pragma once
#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "../../Common/Camera.h"
#include "FrameResource.h"
#include "Waves.h"
#include <ctime>
#include "SceneNode.h"
#include "Aircraft.h"
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class World
{
public:
	explicit							World();
	void								update(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList);
	void								draw();

public:
	void								loadTextures(std::unordered_map<std::string, std::unique_ptr<Texture>>& Textures, Microsoft::WRL::ComPtr<ID3D12Device> Device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList);
	void								buildScene(std::vector<std::unique_ptr<RenderItem>>& renderList, std::unordered_map<std::string, std::unique_ptr<Material>>& Materials,
													std::unordered_map<std::string, std::unique_ptr<Texture>>& Textures,
													std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& Geometries,
													std::vector<RenderItem*> RitemLayer[]);

	
private:
	enum Layer
	{
		Background,
		Air,
		LayerCount
	};


private:
	//TextureHolder						mTextures;

	SceneNode							mSceneGraph;
	std::array<SceneNode*, LayerCount>	mSceneLayers;

	XMFLOAT4					mWorldBounds;
	XMVECTOR						mSpawnPosition;
	float								mScrollSpeed;
	Aircraft* mPlane;
	Aircraft* leftPlane;
	Aircraft* rightPlane;
	Entity background;
};

