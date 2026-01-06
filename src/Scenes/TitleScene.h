#pragma once
#include "Effects/Fade.h"
#include "KamataEngine.h"
#include <vector>

class Skydome;

/// <summary>
/// タイトルシーン
/// </summary>
class TitleScene {
private:
	// シーンのフェーズ
	enum class Phase {
		kFadeIn,  // フェードイン
		kMain,    // メイン処理
		kFadeOut, // フェードアウト
	};

	// 3Dモデル
	KamataEngine::Model* modelTitle_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::Model* modelShooterEnemy_ = nullptr;
	KamataEngine::Model* modelChasingEnemy_ = nullptr;
	KamataEngine::Model* skydomeModel_ = nullptr;

	uint32_t skydomeTextureHandle_ = 0;

	// 色変更オブジェクト
	KamataEngine::ObjectColor objectColorTitle_;
	// 色の数値
	KamataEngine::Vector4 colorTitle_;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransformTitle_;  // タイトル文字用
	KamataEngine::WorldTransform worldTransformPlayer_; // プレイヤー用

	// ★変更：敵のTransformを4体分の配列にする
	KamataEngine::WorldTransform worldTransformEnemies_[4];

	float initialTitleY_ = 0.0f;  // タイトル文字の初期Y座標
	float animationTimer_ = 0.0f; // アニメーション用カウンタ

	// 3Dオブジェクト
	Skydome* skydome_ = nullptr;

	// カメラ
	KamataEngine::Camera camera_;

	// アニメーション用の角度
	float viewAngle_ = 0.0f;

	// 終了フラグ
	bool finished_ = false;

	// フェード
	Fade* fade_ = nullptr;

	// 現在のフェーズ
	Phase phase_ = Phase::kFadeIn;

	// --- プレイヤー演出用変数 ---
	float playerVelocityY_ = 0.0f; // プレイヤーの落下速度
	bool isLanding_ = false;       // 着地したかどうかのフラグ
	float bounceTimer_ = 0.0f;     // バウンド（ぽよん）演出用タイマー

	// 敵のアニメーション用タイマー
	float enemyTimer_ = 0.0f;

public:
	// デストラクタ
	~TitleScene();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// 終了したかを取得する
	bool GetIsFinished() const { return finished_; }
	void SetIsFinished(bool finished) { finished_ = finished; }
};