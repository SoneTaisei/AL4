#include "Objects/ChasingEnemy.h"
#include "System/MapChipField.h"
#include "Utils/TransformUpdater.h"
#include <array>
#include <numbers>
#include <cmath>
#include "Player.h"
#include <algorithm>

using namespace KamataEngine;

Vector3 ChasingEnemy::CornerPosition(const Vector3 center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {kWidth / 2.0f,  -kHeight / 2.0f, 0},
        {-kWidth / 2.0f, -kHeight / 2.0f, 0},
        {kWidth / 2.0f,  kHeight / 2.0f,  0},
        {-kWidth / 2.0f, kHeight / 2.0f,  0}
    };
	return center + offsetTable[static_cast<uint32_t>(corner)];
}

void ChasingEnemy::MapCollisionUp(CollisionMapInfo& info) {
	if (info.move.y <= 0) return;
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	bool hit = false;
	MapChipField::IndexSet impacted = {UINT32_MAX, UINT32_MAX};
	auto idxL = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	if (mapChipField_->GetMapChipTypeByIndex(idxL.xIndex, idxL.yIndex) == MapChipType::kBlock) { hit = true; impacted = idxL; }
	auto idxR = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	if (mapChipField_->GetMapChipTypeByIndex(idxR.xIndex, idxR.yIndex) == MapChipType::kBlock) { hit = true; if (impacted.xIndex == UINT32_MAX) impacted = idxR; }
	if (hit) {
		info.isCeilingHit = true;
		auto rect = mapChipField_->GetRectByIndex(impacted.xIndex, impacted.yIndex);
		float targetCenterY = rect.bottom - kHeight / 2.0f;
		info.move.y = targetCenterY - worldTransform_.translation_.y;
	}
}

void ChasingEnemy::MapCollisionDown(CollisionMapInfo& info) {
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	bool hit = false;
	MapChipField::IndexSet impacted = {UINT32_MAX, UINT32_MAX};
	float deepestPen = -FLT_MAX;
	auto idxLB = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	if (mapChipField_->GetMapChipTypeByIndex(idxLB.xIndex, idxLB.yIndex) == MapChipType::kBlock) {
		auto rect = mapChipField_->GetRectByIndex(idxLB.xIndex, idxLB.yIndex);
		if (positionsNew[kLeftBottom].y < rect.top) { hit = true; impacted = idxLB; deepestPen = positionsNew[kLeftBottom].y; }
	}
	auto idxRB = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	if (mapChipField_->GetMapChipTypeByIndex(idxRB.xIndex, idxRB.yIndex) == MapChipType::kBlock) {
		auto rect = mapChipField_->GetRectByIndex(idxRB.xIndex, idxRB.yIndex);
		if (positionsNew[kRightBottom].y < rect.top) {
			hit = true;
			if (impacted.xIndex == UINT32_MAX || positionsNew[kRightBottom].y < deepestPen) impacted = idxRB;
		}
	}
	if (hit) {
		info.isLanding = true;
		auto rect = mapChipField_->GetRectByIndex(impacted.xIndex, impacted.yIndex);
		float targetCenterY = rect.top + kHeight / 2.0f;
		info.move.y = targetCenterY - worldTransform_.translation_.y;
	}
}

void ChasingEnemy::MapCollisionRight(CollisionMapInfo& info) {
	if (info.move.x <= 0) return;
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + info.move;
	Vector3 rightTopCheck = centerNew + Vector3{kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 rightBottomCheck = centerNew + Vector3{kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	auto idxTop = mapChipField_->GetMapChipIndexSetByPosition(rightTopCheck);
	auto idxBottom = mapChipField_->GetMapChipIndexSetByPosition(rightBottomCheck);
	if ((mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock) ||
	    (mapChipField_->GetMapChipTypeByIndex(idxBottom.xIndex, idxBottom.yIndex) == MapChipType::kBlock)) {
		info.isWallContact = true;
		auto idx = (mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock) ? idxTop : idxBottom;
		auto rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		info.move.x = rect.left - kWidth / 2.0f - worldTransform_.translation_.x;
	}
}

void ChasingEnemy::MapCollisionLeft(CollisionMapInfo& info) {
	if (info.move.x >= 0) return;
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + info.move;
	Vector3 leftTopCheck = centerNew + Vector3{-kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 leftBottomCheck = centerNew + Vector3{-kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	auto idxTop = mapChipField_->GetMapChipIndexSetByPosition(leftTopCheck);
	auto idxBottom = mapChipField_->GetMapChipIndexSetByPosition(leftBottomCheck);
	if ((mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock) ||
	    (mapChipField_->GetMapChipTypeByIndex(idxBottom.xIndex, idxBottom.yIndex) == MapChipType::kBlock)) {
		info.isWallContact = true;
		auto idx = (mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock) ? idxTop : idxBottom;
		auto rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		info.move.x = rect.right + kWidth / 2.0f - worldTransform_.translation_.x;
	}
}

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
			if (nd != lrDirection_) {
				lrDirection_ = nd;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = 0.0f;
			}
		}
	}

	// 3. 地上か空中かで速度を適用する
	if (onGround_) {
		// 地上ならX速度を更新
		velocity_.x = desiredX;
	} else {
		// 空中ならY速度（重力）のみ更新
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::fmaxf(velocity_.y, -kLimitFallSpeed);
		// (メモ: 空中でのX速度は前のフレームのものを維持する形になる)
	}

	// 衝突と移動（X）
	CollisionMapInfo infoX{};
	infoX.move = {velocity_.x, 0.0f, 0.0f};
	MapCollisionRight(infoX);
	MapCollisionLeft(infoX);
	if (infoX.isWallContact) {
		LRDirection newDir = (lrDirection_ == LRDirection::kLeft) ? LRDirection::kRight : LRDirection::kLeft;
		if (lrDirection_ != newDir) {
			lrDirection_ = newDir;
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = 0.0f;
		}
	}
	worldTransform_.translation_.x += infoX.move.x;

	// Y
	CollisionMapInfo infoY{};
	infoY.move = {0.0f, velocity_.y, 0.0f};
	MapCollisionUp(infoY);
	MapCollisionDown(infoY);
	worldTransform_.translation_.y += infoY.move.y;
	if (infoY.isLanding) { onGround_ = true; velocity_.y = 0; }
	else onGround_ = false;
	if (infoY.isCeilingHit) velocity_.y = 0;

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
			onGround_ = false;
		}
	}
}
