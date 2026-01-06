#pragma once
#include "KamataEngine.h"

class Player;

class UI {
public:
	void Initialize();

	void Update();

	// ポーズメニュー用の入力処理を UI が行う
	// 引数:
	// - pauseMenuIndex: 現在の選択インデックス（0..kPauseMenuCount-1）。UI 内で上下移動を行う。
	// - outConfirm: 決定が押されたら true に設定される（呼び出し側で処理する）
	// - outCancel: キャンセルが押されたら true に設定される（呼び出し側で処理する）
	void HandlePauseInput(int& pauseMenuIndex, bool& outConfirm, bool& outCancel);

	void Draw(int pauseMenuIndex);

private:
	uint32_t pausdHandle_;
	uint32_t backGroundHnadle_;
	uint32_t arrowHnadle_;
	KamataEngine::Sprite* pausdSprite_ = {};
	KamataEngine::Sprite* backGroundSprite_ = {};
	KamataEngine::Sprite* arrowSprite_ = {};
	KamataEngine::Vector2 pausdSpritePosition_ = {};

	// 最後に使われた入力がコントローラか（true = controller, false = keyboard）
	bool lastInputIsGamepad_ = false;

	// スティック判定閾値（コントローラ入力検出用）
	static inline const float kGamepadStickThreshold = 0.3f;

	// ポーズメニューの項目数（GameScene と合わせる）
	static inline const int kPauseMenuCount = 2;
};
