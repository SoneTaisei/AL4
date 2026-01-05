#include "Objects/ChasingEnemy.h"
#include "System/MapChipField.h"
#include "Utils/TransformUpdater.h"
#include <array>
#include <numbers>
#include <cmath>
#include "Player.h"
#include <algorithm>

using namespace KamataEngine;

void ChasingEnemy::Initialize(Model* model, uint32_t textureHandle, Camera* camera, const Vector3& position) {
#ifdef _DEBUG
	assert(model);
#endif
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	worldTransform_.scale_ = {kInitialScale, kInitialScale, kInitialScale};

	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	velocity_ = {-kPatrolSpeed, 0.0f, 0.0f};
	workTimer_ = 0.0f;
	state_ = State::kAlive;
	objectColor_.Initialize();
	color_ = {1.0f,1.0f,1.0f,1.0f};
	deathTimer_ = 0.0f;
}

void ChasingEnemy::Update() {
	if (state_ == State::kDead) return;

	const float dt = 1.0f / 60.0f;
	if (state_ == State::kDying) {
		float total = kDeathSpinDuration + kDeathShrinkDuration;
		deathTimer_ += dt;
		if (deathTimer_ < kDeathSpinDuration) {
			worldTransform_.rotation_.y += kDeathSpinSpeed * dt;
		} else {
			float shrinkElapsed = deathTimer_ - kDeathSpinDuration;
			float t = std::clamp(shrinkElapsed / kDeathShrinkDuration, 0.0f, 1.0f);
			float scale = (1.0f - t) * kInitialScale;
			worldTransform_.scale_ = {scale, scale, scale};
			color_.w = 1.0f - t;
		}
		TransformUpdater::WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		if (deathTimer_ >= total) state_ = State::kDead;
		return;
	}

	// 1. まずデフォルトの速度（パトロール）を設定
	float desiredX = (lrDirection_ == LRDirection::kLeft) ? -kPatrolSpeed : kPatrolSpeed;
	float desiredY = 0.0f; // Y軸のデフォルト速度は0

	// 2. プレイヤーが範囲内か検知する（★onGroundに関係なく実行）
	if (targetPlayer_ != nullptr) {
		Vector3 myPos = GetWorldPosition();
		Vector3 pPos = targetPlayer_->GetWorldPosition();
		float dx = pPos.x - myPos.x;
		float dy = pPos.y - myPos.y;

		// 簡易検出（距離矩形）
		if (std::fabs(dx) <= kDetectRange && std::fabs(dy) <= kDetectRange) {
			// 追尾速度と向きを設定
			desiredX = (dx > 0.0f) ? kChaseSpeed : -kChaseSpeed;
			LRDirection nd = (dx > 0.0f) ? LRDirection::kRight : LRDirection::kLeft;
			if (nd != lrDirection_ && turnTimer_ >= 1.0f) {
				lrDirection_ = nd;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = 0.0f;
			}
			//   (dx, dy) というベクトルを求める
			float length = std::sqrt(dx * dx + dy * dy);

			// ゼロ除算を避けつつ、正規化して kChaseSpeed を掛ける
			if (length > 0.001f) {
				float invLength = 1.0f / length;
				desiredX = (dx * invLength) * kChaseSpeed;
				desiredY = (dy * invLength) * kChaseSpeed;
			} else {
				// プレイヤーと重なっている場合は停止
				desiredX = 0.0f;
				desiredY = 0.0f;
			}
		}
	}

	// 3. 速度を決定
	velocity_.x = desiredX;
	velocity_.y = desiredY; // ★Y軸の速度を反映
	velocity_.z = 0;

	// 4. 速度を座標に反映する (★前回の修正点)
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y; // ★Y軸の座標を更新
	worldTransform_.translation_.z += velocity_.z;

	// 歩行アニメーション
	workTimer_ += dt;
	float timeInCycle = std::fmod(workTimer_, kWalkMotionTime);
	float progress = timeInCycle / kWalkMotionTime;
	float sinArg = progress * 2.0f * std::numbers::pi_v<float>;
	float r = (std::sin(sinArg) + 1.0f) / 2.0f;
	worldTransform_.rotation_.x = (1.0f - r) * kWalkMotionAngleStart + r * kWalkMotionAngleEnd;

	// 旋回補間
	if (turnTimer_ < 1.0f) {
		turnTimer_ += 1.0f / (60.0f * kTimeTurn);
		turnTimer_ = std::fminf(turnTimer_, 1.0f);
		float destTable[] = { std::numbers::pi_v<float> * 0.5f, std::numbers::pi_v<float> * 1.5f };
		float dest = destTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotation_.y = Lerp(turnFirstRotationY_, dest, turnTimer_);
	}

	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void ChasingEnemy::Draw() {
	if (state_ == State::kDead) return;
	DirectXCommon* dx = DirectXCommon::GetInstance();
	Model::PreDraw(dx->GetCommandList());
	if (state_ == State::kDying) {
		// objectColor_.SetColor(color_); // 必要なら有効化
		model_->Draw(worldTransform_, *camera_, textureHandle_, &objectColor_);
	} else {
		model_->Draw(worldTransform_, *camera_, textureHandle_);
	}
	Model::PostDraw();
}

Vector3 ChasingEnemy::GetWorldPosition() {
	Vector3 p;
	p.x = worldTransform_.matWorld_.m[3][0];
	p.y = worldTransform_.matWorld_.m[3][1];
	p.z = worldTransform_.matWorld_.m[3][2];
	return p;
}

AABB ChasingEnemy::GetAABB() {
	Vector3 w = GetWorldPosition();
	AABB a;
	a.min = {w.x - (kWidth / 2.0f), w.y - (kHeight / 2.0f), w.z - (kWidth / 2.0f)};
	a.max = {w.x + (kWidth / 2.0f), w.y + (kHeight / 2.0f), w.z + (kWidth / 2.0f)};
	return a;
}

void ChasingEnemy::OnCollision(const Player* player) {
	(void)player;
	// プレイヤーに触れたときの処理を追加してください
}

void ChasingEnemy::SetIsAlive(bool isAlive) {
	if (isAlive) {
		state_ = State::kAlive;
		deathTimer_ = 0.0f;
		worldTransform_.scale_ = {kInitialScale,kInitialScale,kInitialScale};
		color_ = {1,1,1,1};
	} else {
		if (state_ == State::kAlive) {
			state_ = State::kDying;
			deathTimer_ = 0.0f;
			velocity_ = {0,0,0};
		}
	}
}
