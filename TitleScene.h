#pragma once
#include "KamataEngine.h"
#include "Fade.h"
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
	KamataEngine::Model* skydomeModel_ = nullptr;

	uint32_t skydomeTextureHandle_ = 0;

	// 色変更オブジェクト
	KamataEngine::ObjectColor objectColorTitle_;
	// 色の数値
	KamataEngine::Vector4 colorTitle_;
	// テクスチャ（今回は使いませんが、将来のために宣言だけしておくのも良いでしょう）
	// uint32_t textureHandle_ = 0;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransformTitle_;  // タイトル文字用
	KamataEngine::WorldTransform worldTransformPlayer_; // プレイヤー用

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
	bool IsFinished() const { return finished_; }
};
