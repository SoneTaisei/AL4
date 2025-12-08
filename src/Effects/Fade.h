#pragma once
#include "KamataEngine.h"

/// <summary>
/// フェード
/// </summary>
class Fade {
public:
	enum class Status {
		None,    // フェードなし
		FadeIn,  // フェードイン中（暗→明）
		FadeOut, // フェードアウト中（明→暗）
	};

private:
	// テクスチャハンドル
	uint32_t textureHandle_ = 0;

	// スプライト
	KamataEngine::Sprite* sprite_ = nullptr;

	// 現在のフェードの状態
	Status status_ = Status::None;
	// フェードの持続時間
	float duration_ = 0.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;

public:
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

	// フェード開始
	void Start(Status status, float duration);

	// フェード終了
	void Stop();

	// フェードが終了したか判定
	bool IsFinished() const;
};