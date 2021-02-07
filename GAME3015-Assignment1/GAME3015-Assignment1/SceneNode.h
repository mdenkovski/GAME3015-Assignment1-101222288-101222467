#pragma once

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

class SceneNode
{
public:
	typedef std::unique_ptr<SceneNode> Ptr;


public:
	SceneNode();

	void					attachChild(Ptr child);
	Ptr						detachChild(const SceneNode& node);

	void					update(GameTimer dt);

	XMFLOAT3			getWorldPosition() const;
	XMFLOAT3			getWorldTransform() const;


private:
	virtual void			updateCurrent(GameTimer dt);
	void					updateChildren(GameTimer dt);

	virtual void			draw() const;
	virtual void			drawCurrent() const;
	void					drawChildren() const;


private:
	std::vector<Ptr>		mChildren;
	SceneNode* mParent;
};

