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

void Entity::updateCurrent(GameTimer dt, RenderItem* renderItem)
{
	XMVECTOR displacement = mVelocity * dt.DeltaTime();
	mPosition = XMVectorAdd(mPosition, displacement);
	renderItem = std::move(renderItem);
	renderItem->NumFramesDirty = 1;
	XMStoreFloat4x4(&renderItem->World, XMMatrixScaling(1,1,1) * XMMatrixTranslationFromVector(mPosition));
	renderItem = std::move(renderItem);

}