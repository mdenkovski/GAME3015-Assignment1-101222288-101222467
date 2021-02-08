#pragma once
#include "SceneNode.h"


class Entity : public SceneNode
{

public:
	void setVelocity(XMVECTOR velocity);
	void setVelocity(float vx, float vy, float vz);
	XMVECTOR getVelocity() const;

	virtual	void		updateCurrent(GameTimer dt, RenderItem* renderItem);
private:
	XMVECTOR mVelocity;
	XMVECTOR mPosition;

//public:
//	void				setVelocity(sf::Vector2f velocity);
//	void				setVelocity(float vx, float vy);
//	sf::Vector2f		getVelocity() const;
//
//	virtual	void		updateCurrent(sf::Time dt);
//
//public:
//	sf::Vector2f		mVelocity;
};

