#pragma once
#include "KamataEngine.h"

class Player;

class HUD {
public:
	void Initialize();
	void Update();
	void Draw(const Player* player);

	// ステージ番号表示
	void DrawStageNumber(int stageNo);

private:
	uint32_t hpHandle_;
	KamataEngine::Sprite* hpSprite_[3] = {};
	KamataEngine::Vector2 hpSpritePosition_[3] = {};
	// 背景用（黒くする用）のスプライト配列
	KamataEngine::Sprite* hpBackSprite_[3] = {};

	//「STAGE 1-」の文字用
	uint32_t stageTextHandle_ = 0;
	KamataEngine::Sprite* stageTextSprite_ = nullptr;

	// 数字（0～9）用
	// 数字の画像を配列で管理します（インデックスがそのまま数字に対応）
	uint32_t numberHandles_[10] = {};
	KamataEngine::Sprite* numberSprites_[10] = {};
};