#pragma once
#include "KamataEngine.h"
#include <vector>

// 前方宣言
class Skydome;
class Fade;

class StageSelectScene {
private:
	enum class Phase {
		kFadeIn,  // フェードイン
		kMain,    // メイン処理
		kFadeOut, // フェードアウト
	};

	KamataEngine::Camera camera_;

	// --- 3Dモデル ---
	KamataEngine::Model* skydomeModel_ = nullptr;
	KamataEngine::Model* stageCubeModel_ = nullptr;
	KamataEngine::Model* cursorModel_ = nullptr;

	// --- 3Dオブジェクト ---
	Skydome* skydome_ = nullptr;
	std::vector<KamataEngine::WorldTransform*> stageCubeTransforms_;
	KamataEngine::WorldTransform* cursorTransform_ = nullptr;

	// ステージの総数
	int maxStages_ = 10;
	// 現在選択しているステージ番号 (0-indexed)
	int selectedStageIndex_ = 0;

	// カーソルの行 (0:上段, 1:下段)
	int cursorRow_ = 0;
	// カーソルの列 (0～4)
	int cursorCol_ = 0;

	// 現在のフェーズ
	Phase phase_ = Phase::kFadeIn;
	// フェード
	Fade* fade_ = nullptr;

	// 終了フラグ
	bool finished_ = false;

	std::vector<uint32_t> stageCubeTextureHandles_;

public:
	~StageSelectScene();
	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }
	int GetSelectedStageNo() const { return selectedStageIndex_ + 1; }
};