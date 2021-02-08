#include "SceneNode.h"
#include <algorithm>
#include <cassert>


SceneNode::SceneNode()
	: mChildren()
	, mParent(nullptr)
{
}

void SceneNode::attachChild(Ptr child)
{
	child->mParent = this;
	mChildren.push_back(std::move(child));
}

SceneNode::Ptr SceneNode::detachChild(const SceneNode& node)
{
	auto found = std::find_if(mChildren.begin(), mChildren.end(), [&](Ptr& p) { return p.get() == &node; });
	assert(found != mChildren.end());

	Ptr result = std::move(*found);
	result->mParent = nullptr;
	mChildren.erase(found);
	return result;
}

void SceneNode::update(GameTimer dt)
{
	updateCurrent(dt);
	updateChildren(dt);
}

void SceneNode::updateCurrent(GameTimer  dt)
{
	// Do nothing by default
}

void SceneNode::updateChildren(GameTimer dt)
{
	for (Ptr& child : mChildren)
	{
		child->update(dt);
	}
}

void SceneNode::draw(ID3D12GraphicsCommandList* cmdList, RenderItem* ritems) const
{
	//// Apply transform of current node
	//ritems.transform *= getTransform();

	//// Draw node and children with changed transform
	//drawCurrent(cmdList, ritems);
	//drawChildren(cmdList, ritems);
}

void SceneNode::drawCurrent(ID3D12GraphicsCommandList* cmdList, RenderItem* ritems) const
{
	// Do nothing by default
}

void SceneNode::drawChildren(ID3D12GraphicsCommandList* cmdList, RenderItem* ritems) const
{
	/*for (const Ptr& child : mChildren)
		child->draw(cmdList, ritems);*/
}

XMVECTOR SceneNode::getWorldPosition() const
{
	return getWorldTransform() ;
}

XMVECTOR SceneNode::getWorldTransform() const
{
	XMVECTOR transform = {1.0,1.0,1.0};

	/*for (const SceneNode* node = this; node != nullptr; node = node->mParent)
		transform = node->getTransform() * transform;*/

	return transform;
}
