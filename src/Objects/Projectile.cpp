#include "Projectile.h"
#include <cmath>

using namespace KamataEngine;

void Projectile::Initialize(Model* model, uint32_t textureHandle, Camera* camera,
                            const Vector3& position, const Vector3& velocity, float lifeTime) {
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = { 0.5f, 0.5f, 0.5f };
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	velocity_ = velocity;
	lifeTime_ = lifeTime;
	alive_ = true;
}

void Projectile::Update() {
	if (!alive_) return;

	const float dt = 1.0f / 60.0f;
	// 移動
	worldTransform_.translation_.x += velocity_.x * dt;
	worldTransform_.translation_.y += velocity_.y * dt;
	worldTransform_.translation_.z += velocity_.z * dt;

	// 行列更新
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	// 寿命処理
	lifeTime_ -= dt;
	if (lifeTime_ <= 0.0f) {
		alive_ = false;
	}
}

void Projectile::Draw() {
	if (!alive_) return;
	if (!model_ || !camera_) return;

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	Model::PreDraw(dxCommon->GetCommandList());
	model_->Draw(worldTransform_, *camera_, textureHandle_);
	Model::PostDraw();
}

AABB Projectile::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kRadius / 2.0f, worldPos.y - kRadius / 2.0f, worldPos.z - kRadius / 2.0f};
	aabb.max = {worldPos.x + kRadius / 2.0f, worldPos.y + kRadius / 2.0f, worldPos.z + kRadius / 2.0f};
	return aabb;
}

void Projectile::OnCollision() {
	// プレイヤーに当たったら弾は消える
	alive_ = false;
}
