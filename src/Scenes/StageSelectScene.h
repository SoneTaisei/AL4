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
		kSelected,// セレクトされた後の処理
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
	KamataEngine::Vector3 rotationSpeed = {};
	float selectionJumpHeight_ = 0.0f;
	float jumpVelocity_ = 0.0f;

	// 演出用タイマー（フレーム数をカウント）
	int selectionTimer_ = 0;

	// 演出を何フレーム続けるかの定数（例：60フレーム = 約1秒）
	const int kSelectionDuration = 120;

	// ステージの総数
	int maxStages_ = 3;
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

	bool GetIsFinished() const { return finished_; }
	void SetIsFinished(bool finished) { finished_ = finished; }
	int GetSelectedStageNo() const { return selectedStageIndex_ + 1; }
};