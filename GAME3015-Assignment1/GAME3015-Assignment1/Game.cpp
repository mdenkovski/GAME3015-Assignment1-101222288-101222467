#include "Game.h"

Game::Game(HINSTANCE hInstance)
	:mWorld(hInstance)
{


}

Game::~Game()
{
	
}

int Game::Run()
{
	return mWorld.Run();
}

bool Game::Initialize()
{
	return mWorld.Initialize();
}

void Game::OnResize()
{
	mWorld.OnResize();
}

void Game::Update(const GameTimer& gt)
{
	mWorld.Update(gt);
}

void Game::Draw(const GameTimer& gt)
{
	mWorld.Draw(gt);
}

void Game::OnMouseDown(WPARAM btnState, int x, int y)
{
	mWorld.OnMouseDown(btnState, x, y);
}

void Game::OnMouseUp(WPARAM btnState, int x, int y)
{
	mWorld.OnMouseUp(btnState, x, y);
}

void Game::OnMouseMove(WPARAM btnState, int x, int y)
{
	mWorld.OnMouseMove(btnState, x, y);
}

void Game::OnKeyboardInput(const GameTimer& gt)
{
	mWorld.OnKeyboardInput(gt);
}
