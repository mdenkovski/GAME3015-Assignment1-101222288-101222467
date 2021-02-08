#pragma once
#include "SceneNode.h"
#include <vector>


class Entity : public SceneNode
{

public:
	void setVelocity(XMVECTOR velocity);
	void setVelocity(float vx, float vy, float vz);
	XMVECTOR getVelocity() const;

	virtual	void		updateCurrent(GameTimer dt, std::vector<std::unique_ptr<RenderItem>>& renderList);
public:
	XMVECTOR mVelocity;

};

