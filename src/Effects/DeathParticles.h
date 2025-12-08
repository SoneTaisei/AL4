#pragma once
#include "KamataEngine.h"
#include <array> // std::array を使うために必要

// 前方宣言
class TransformUpdater;

/// <summary>
/// デス演出用パーティクル
/// </summary>
class DeathParticles {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデル</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	/// <param name="camera">カメラ</param>
	/// <param name="position">初期座標</param>
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// パーティクルの再生を開始する
	void Start(const KamataEngine::Vector3& position);

	/// <summary>
	/// 演出が終了したか
	/// </summary>
	/// <returns>終了していたらtrue</returns>
	bool IsFinished() const { return isFinished_; }

private:
	// モデルのポインタ
	KamataEngine::Model* model_ = nullptr;
	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;
	// カメラのポインタ
	KamataEngine::Camera* camera_ = nullptr;

	// パーティクルの個数
	static inline const uint32_t kNumParticles = 8;
	// ワールドトランスフォーム
	std::array<KamataEngine::WorldTransform, kNumParticles> worldTransforms_;

	// 存続時間（消滅までの時間）<秒>
	static inline const float kDuration = 1.0f;
	// 移動の速さ
	static inline const float kSpeed = 0.1f;
	// 分割した1個分の角度（2πラジアン ÷ 8分割）
	static inline const float kAngleUnit = (2.0f * 3.14159265f) / kNumParticles;

	// パーティクルの初期スケール
	static inline const float kInitialScale = 0.3f;

	// 終了フラグ
	bool isFinished_ = false;
	// 経過時間カウント
	float counter_ = 0.0f;

	// 色変更オブジェクト
	KamataEngine::ObjectColor objectColor_;
	// 色の数値
	KamataEngine::Vector4 color_;
};
