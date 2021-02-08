#pragma once
#include "Entity.h"


class Aircraft : public Entity
{
public:
	enum Type
	{
		Eagle,
		Raptor,
	};


public:
	Aircraft();


private:
	virtual void		drawCurrent(GameTimer dt, RenderItem* renderItem) const;


private:
	Type				mType;

	RenderItem* mSprite;
};

