#pragma once

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
//#include "../../Common/UploadBuffer.h"
//#include "../../Common/GeometryGenerator.h"
//#include "../../Common/Camera.h"
//#include "FrameResource.h"
//#include "Waves.h"
//#include <ctime>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

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

class SceneNode 
{
public:
	typedef std::unique_ptr<SceneNode> Ptr;


public:
	SceneNode();

	void					attachChild(Ptr child);
	Ptr						detachChild(const SceneNode& node);

	void					update(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList);

	XMVECTOR			getWorldPosition() const;
	XMVECTOR			getWorldTransform() const;

	std::unique_ptr<RenderItem> renderItem;
	int renderIndex;

	XMVECTOR mPosition;
	XMVECTOR Scale;

private:
	virtual void			updateCurrent(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList);
	void					updateChildren(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList);

	virtual void			draw(ID3D12GraphicsCommandList* cmdList, RenderItem& ritems) ;
	virtual void			drawCurrent(ID3D12GraphicsCommandList* cmdList, RenderItem& ritems) ;
	void					drawChildren(ID3D12GraphicsCommandList* cmdList, RenderItem& ritems) ;


private:
	std::vector<Ptr>		mChildren;
	SceneNode* mParent;
	
};

