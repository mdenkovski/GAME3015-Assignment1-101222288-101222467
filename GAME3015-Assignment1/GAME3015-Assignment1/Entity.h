#pragma once
#include "SceneNode.h"


class Entity : public SceneNode
{

public:
	void setVelocity(Vector3f velocity);
	void setVelocity(float vx, float vy);
	Vector3f getVelocity() const;
private:
	Vector3f mVelocity;


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

