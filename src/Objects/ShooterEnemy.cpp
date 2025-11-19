#include "ShooterEnemy.h"
#include "Utils/TransformUpdater.h"
#include "Player.h"
#include <algorithm>
#include <numbers>
#include <cmath>

using namespace KamataEngine;

void ShooterEnemy::Initialize(Model* model, uint32_t textureHandle, Camera* camera, const Vector3& position) {
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	velocity_ = { -kMoveSpeed, 0.0f, 0.0f };
	shootTimer_ = 0.0f;
}

void ShooterEnemy::MapCollisionRight(Vector3& move) {
	if (!mapChipField_) return;
	if (move.x <= 0) return;
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + move;
	Vector3 rightTopCheck = centerNew + Vector3{kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 rightBottomCheck = centerNew + Vector3{kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	auto idxTop = mapChipField_->GetMapChipIndexSetByPosition(rightTopCheck);
	auto idxBottom = mapChipField_->GetMapChipIndexSetByPosition(rightBottomCheck);
	if ((mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock ||
	     mapChipField_->GetMapChipTypeByIndex(idxBottom.xIndex, idxBottom.yIndex) == MapChipType::kBlock)
	    && lrDirection_ == LRDirection::kRight) {
		// 壁接触 -> 反転
		lrDirection_ = LRDirection::kLeft;
		move.x = 0.0f;
	}
}

void ShooterEnemy::MapCollisionLeft(Vector3& move) {
	if (!mapChipField_) return;
	if (move.x >= 0) return;
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + move;
	Vector3 leftTopCheck = centerNew + Vector3{-kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 leftBottomCheck = centerNew + Vector3{-kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	auto idxTop = mapChipField_->GetMapChipIndexSetByPosition(leftTopCheck);
	auto idxBottom = mapChipField_->GetMapChipIndexSetByPosition(leftBottomCheck);
	if ((mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock ||
	     mapChipField_->GetMapChipTypeByIndex(idxBottom.xIndex, idxBottom.yIndex) == MapChipType::kBlock)
	    && lrDirection_ == LRDirection::kLeft) {
		lrDirection_ = LRDirection::kRight;
		move.x = 0.0f;
	}
}

void ShooterEnemy::Update() {
	const float dt = 1.0f / 60.0f;

	// 左右速度は向きに従う
	velocity_.x = (lrDirection_ == LRDirection::kLeft) ? -kMoveSpeed : kMoveSpeed;

	// 移動とマップ衝突（左右のみ簡易）
	Vector3 move = { velocity_.x, 0.0f, 0.0f };
	MapCollisionRight(move);
	MapCollisionLeft(move);
	worldTransform_.translation_.x += move.x;

	// 発射タイマー
	shootTimer_ += dt;
	if (shootTimer_ >= kShootInterval && player_ && model_) {
		shootTimer_ = 0.0f;
		// プレイヤーの位置へ向けて弾を作る
		Vector3 enemyPos = { worldTransform_.matWorld_.m[3][0], worldTransform_.matWorld_.m[3][1], worldTransform_.matWorld_.m[3][2] };
		Vector3 playerPos = player_->GetWorldPosition();
		Vector3 dir = { playerPos.x - enemyPos.x, playerPos.y - enemyPos.y, playerPos.z - enemyPos.z };
		// 正規化
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len > 0.001f) {
			dir.x /= len; dir.y /= len; dir.z /= len;
		}
		Vector3 vel = { dir.x * kProjectileSpeed, dir.y * kProjectileSpeed, dir.z * kProjectileSpeed };

		auto p = std::make_unique<Projectile>();
		p->Initialize(model_, textureHandle_, camera_, enemyPos, vel, 5.0f);
		projectiles_.push_back(std::move(p));
	}

	// 弾の更新と掃除
	for (auto& p : projectiles_) {
		if (p && p->IsAlive()) p->Update();
	}
	projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
		[](const std::unique_ptr<Projectile>& p) { return !p || !p->IsAlive(); }), projectiles_.end());

	// 行列更新
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void ShooterEnemy::Draw() {
	// 敵本体
	if (model_ && camera_) {
		DirectXCommon* dxCommon = DirectXCommon::GetInstance();
		Model::PreDraw(dxCommon->GetCommandList());
		model_->Draw(worldTransform_, *camera_, textureHandle_);
		Model::PostDraw();
	}

	// 弾
	for (auto& p : projectiles_) {
		if (p && p->IsAlive()) p->Draw();
	}
}

bool ShooterEnemy::GetHasAlive() const {
	// 敵本体は常に存在する想定。弾が残っているかだけ返す例。
	for (auto& p : projectiles_) if (p && p->IsAlive()) return true;
	return true;
}