#include "Entity.h"

void Entity::setVelocity(XMVECTOR velocity)
{
	mVelocity = velocity;
}

void Entity::setVelocity(float vx, float vy, float vz)
{
	mVelocity = XMVECTOR{ vx, vy, vz };
}

XMVECTOR Entity::getVelocity() const
{
	return mVelocity;
}

void Entity::updateCurrent(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList)
{
	XMVECTOR displacement = mVelocity * dt.DeltaTime();
	mPosition = XMVectorAdd(mPosition, displacement);
	renderItem = std::move(renderList[renderIndex]);
	renderItem->NumFramesDirty = 1;
	XMStoreFloat4x4(&renderItem->World, XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslationFromVector(mPosition));
	renderList[renderIndex] = std::move(renderItem);

}