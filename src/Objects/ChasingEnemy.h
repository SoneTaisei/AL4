#pragma once
#include "KamataEngine.h"
#include <numbers>
#include "System/Collision.h"
#include "System/MapChipField.h" // 追加

// 循環参照を避けるための前方宣言
class Player;
class MapChipField;

/// <summary>
/// プレイヤーを追尾する敵
/// </summary>
class ChasingEnemy {
private:
	enum class LRDirection { kRight, kLeft };

	struct CollisionMapInfo {
		bool isCeilingHit = false;
		bool isLanding = false;
		bool isWallContact = false;
		KamataEngine::Vector3 move;
	};

	LRDirection lrDirection_ = LRDirection::kLeft;

	// 旋回
	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;
	static inline const float kTimeTurn = 0.2f;

	// ワールド情報
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;
	KamataEngine::Camera* camera_ = nullptr;

	// 速度 / 追尾パラメータ
	static inline const float kPatrolSpeed = 0.05f;
	static inline const float kChaseSpeed = 0.08f;
	static inline const float kDetectRange = 18.0f;

	KamataEngine::Vector3 velocity_ = {};

	bool onGround_ = false;

	static inline const float kGravityAcceleration = 0.02f;
	static inline const float kLimitFallSpeed = 0.6f;

	MapChipField* mapChipField_ = nullptr;
	const Player* targetPlayer_ = nullptr;

	// 追加: 生成時のマップインデックスを保存
	MapChipField::IndexSet spawnIndex_{UINT32_MAX, UINT32_MAX};

	// 角（当たり判定）
	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	// 生存状態
	enum class State { kAlive, kDying, kDead };
	State state_ = State::kAlive;

	// 死亡演出
	float deathTimer_ = 0.0f;
	static inline const float kDeathSpinDuration = 0.6f;
	static inline const float kDeathShrinkDuration = 0.4f;
	static inline const float kDeathSpinSpeed = 20.0f;
	static inline const float kInitialScale = 1.0f;
	KamataEngine::ObjectColor objectColor_;
	KamataEngine::Vector4 color_;

	// アニメーション
	static inline const float kWalkMotionAngleStart = -15.0f * (std::numbers::pi_v<float> / 180.0f);
	static inline const float kWalkMotionAngleEnd = 15.0f * (std::numbers::pi_v<float> / 180.0f);
	static inline const float kWalkMotionTime = 1.0f;
	float workTimer_ = 0.0f;

	// 当たり判定サイズ（Player に合わせる）
	static inline const float kWidth = 1.9f;
	static inline const float kHeight = 1.9f;

	// ユーティリティ
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3 center, Corner corner);
	void MapCollisionUp(CollisionMapInfo& info);
	void MapCollisionDown(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);

	KamataEngine::Vector3 GetWorldPosition();

public:
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	void SetTargetPlayer(const Player* player) { targetPlayer_ = player; }

	// 追加: 生成時のマップインデックスをセット/取得
	void SetSpawnIndex(const MapChipField::IndexSet& idx) { spawnIndex_ = idx; }
	MapChipField::IndexSet GetSpawnIndex() const { return spawnIndex_; }

	AABB GetAABB();
	void OnCollision(const Player* player);

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	bool GetIsAlive() const { return state_ == State::kAlive; }
	void SetIsAlive(bool isAlive);
};
