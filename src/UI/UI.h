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

	// --- Controller UI resources ---
	uint32_t aHandle_ = 0;
	uint32_t xHandle_ = 0;
	uint32_t selectHandle_ = 0;
	KamataEngine::Sprite* aSprite_ = nullptr;
	KamataEngine::Sprite* xSprite_ = nullptr;
	KamataEngine::Sprite* selectSprite_ = nullptr;

	// 最後に使われた入力がコントローラか（true = controller, false = keyboard）
	bool lastInputIsGamepad_ = false;

	// スティック判定閾値（コントローラ入力検出用）
	static inline const float kGamepadStickThreshold = 0.3f;
};
