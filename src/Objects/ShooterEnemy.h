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
	static inline const float kShootInterval = 2.0f;
	float shootTimer_ = 0.0f;
	static inline const float kProjectileSpeed = 0.45f;

	// 衝突チェック用幅（簡易）
	static inline const float kWidth = 1.9f;
	static inline const float kHeight = 1.9f;

	// マップ衝突（左右のみ）
	void MapCollisionRight(KamataEngine::Vector3& move);
	void MapCollisionLeft(KamataEngine::Vector3& move);

public:
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void SetMapChipField(MapChipField* field) { mapChipField_ = field; }
	void SetPlayer(const Player* player) { player_ = player; }

	void Update();
	void Draw();

	// 生存判定（弾の管理があるため単純）
	bool GetHasAlive() const;
};