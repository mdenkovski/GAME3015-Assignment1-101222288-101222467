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

