#pragma once
#include "KamataEngine.h"
#include "System/MapChipField.h"
#include "Projectile.h"
#include <vector>
#include <memory>

// 前方宣言
class Player;

class ShooterEnemy {
private:
	enum class LRDirection { kRight, kLeft };

	enum class State {
		kAlive, // 生存
		kDying, // 死亡演出中
		kDead   // 完全停止（弾がなくなれば削除）
	};
	State state_ = State::kAlive;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	KamataEngine::Camera* camera_ = nullptr;

	MapChipField* mapChipField_ = nullptr;
	const Player* player_ = nullptr;

	LRDirection lrDirection_ = LRDirection::kLeft;
	KamataEngine::Vector3 velocity_{};
	static inline const float kMoveSpeed = 0.06f;

	// 射撃関連
	std::vector<std::unique_ptr<Projectile>> projectiles_;
	static inline const float kShootInterval = 5.0f;

	// 攻撃予兆を開始する時間（発射の何秒前から膨らみ始めるか）
	static inline const float kChargeDuration = 1.0f;
	// 最大まで膨らんだときの倍率
	static inline const float kMaxChargeScale = 1.5f;
	// 発射後に元の大きさに戻るまでの時間（反動時間）
	static inline const float kRecoilDuration = 0.1f;

	float shootTimer_ = 0.0f;
	static inline const float kProjectileSpeed = 5.0f;

	// 衝突チェック用幅（簡易）
	static inline const float kWidth = 1.9f;
	static inline const float kHeight = 1.9f;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;
	// 旋回時間<秒> (Enemyに合わせて設定)
	static inline const float kTimeTurn = 0.2f;

	// --- 死亡演出用定数と変数 ---
	float deathTimer_ = 0.0f;
	// 回転時間（秒）
	static inline const float kDeathSpinDuration = 0.6f;
	// 縮小（フェード）時間（秒）
	static inline const float kDeathShrinkDuration = 0.4f;
	// 回転速度（ラジアン/秒）
	static inline const float kDeathSpinSpeed = 20.0f;
	// 縮小開始時スケール（通常1.0）
	static inline const float kInitialScale = 1.0f;

	// 色変更オブジェクト（フェードに使用）
	KamataEngine::ObjectColor objectColor_;
	// 色データ RGBA
	KamataEngine::Vector4 color_;

	// マップ衝突（左右のみ）
	void MapCollisionRight(KamataEngine::Vector3& move);
	void MapCollisionLeft(KamataEngine::Vector3& move);

	// 弾用のモデルとテクスチャハンドル
	KamataEngine::Model* projectileModel_ = nullptr;
	uint32_t projectileTextureHandle_ = 0;

public:
	// 引数に projectileModel と projectileTextureHandle を追加
	void Initialize(
	    KamataEngine::Model* model, KamataEngine::Model* projectileModel, uint32_t textureHandle, uint32_t projectileTextureHandle, KamataEngine::Camera* camera,
	    const KamataEngine::Vector3& position);
	void SetMapChipField(MapChipField* field) { mapChipField_ = field; }
	void SetPlayer(const Player* player) { player_ = player; }

	void Update();
	void Draw();

	// 生存判定（弾の管理があるため単純）
	bool GetHasAlive() const;
	// 生存状態をセット（false を渡すと死亡アニメーションを開始）
	void SetIsAlive(bool isAlive);
	bool GetIsAlive() const { return state_ == State::kAlive; }

	// AABBを取得
	AABB GetAABB();
	// 衝突応答
	void OnCollision(const Player* player);

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// GameScene側で弾との当たり判定を行うためにリストを公開
	const std::vector<std::unique_ptr<Projectile>>& GetProjectiles() const { return projectiles_; }
};