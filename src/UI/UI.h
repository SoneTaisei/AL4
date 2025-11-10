#pragma once
#include "KamataEngine.h"

class Player;

class UI {
public:
	void Initialize();

	void Update();

	void Draw(int pauseMenuIndex);

private:
	uint32_t pausdHandle_;
	uint32_t backGroundHnadle_;
	uint32_t arrowHnadle_;
	KamataEngine::Sprite* pausdSprite_ = {};
	KamataEngine::Sprite* backGroundSprite_ = {};
	KamataEngine::Sprite* arrowSprite_ = {};
	KamataEngine::Vector2 pausdSpritePosition_ = {};
};
