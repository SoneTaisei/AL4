#pragma once
#include "KamataEngine.h"

class Player;

class HUD {
public:
	void Initialize();

	void Update();

	void Draw(const Player* player);

private:
	uint32_t hpHandle_;
	KamataEngine::Sprite* hpSprite_[3] = {};
	KamataEngine::Vector2 hpSpritePosition_[3] = {};
};
