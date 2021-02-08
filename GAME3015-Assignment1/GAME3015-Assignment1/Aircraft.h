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
	Aircraft(Type type);

	void Update();


private:
	virtual void		drawCurrent(GameTimer dt) ;


private:
	Type				mType;

};

