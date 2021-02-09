#pragma once
#include "World.h"

class Game
{
public:
	Game(HINSTANCE hInstance);
	Game(const Game& rhs) = delete;
	Game& operator=(const Game& rhs) = delete;
	~Game();
	virtual int Run();
	virtual bool Initialize();

private:
	virtual void OnResize();
	virtual void Update(const GameTimer& gt);
	virtual void Draw(const GameTimer& gt);

	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

	void OnKeyboardInput(const GameTimer& gt);

private:
	World mWorld;
};

