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

	void Update();


private:
	virtual void		drawCurrent(GameTimer dt) ;


private:
	Type				mType;

};

