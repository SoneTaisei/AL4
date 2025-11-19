#pragma once
#include "KamataEngine.h"
#include "Utils/TransformUpdater.h"
#include "System/MapChipField.h"

class Projectile {
private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::Vector3 velocity_{};
	float lifeTime_ = 0.0f; // 残り寿命（秒）
	bool alive_ = false;

public:
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera,
	                const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, float lifeTime = 5.0f);

	void Update();
	void Draw();

	bool IsAlive() const { return alive_; }

	// ワールド座標取得（必要なら）
	KamataEngine::Vector3 GetWorldPosition() const {
		return { worldTransform_.matWorld_.m[3][0], worldTransform_.matWorld_.m[3][1], worldTransform_.matWorld_.m[3][2] };
	}
};