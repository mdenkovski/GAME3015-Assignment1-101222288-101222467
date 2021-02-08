#include "Aircraft.h"


Aircraft::Aircraft()
{
	
}

void Aircraft::Update()
{
	if (XMVectorGetX(mPosition) > 1.8 || XMVectorGetX(mPosition) < -1.8)
	{
		mVelocity *= -1;
	}
}

void Aircraft::drawCurrent(GameTimer dt) 
{
}