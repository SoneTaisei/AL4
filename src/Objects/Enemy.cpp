#include "Enemy.h"
#include "Utils/Easing.h"
#include "System/MapChipField.h"
#include "Utils/TransformUpdater.h"
#include <algorithm>
#include <array>
#include <numbers>
#include <cmath>
#include "Player.h"

using namespace KamataEngine;

KamataEngine::Vector3 Enemy::CornerPosition(const KamataEngine::Vector3 center, Corner corner) {
	KamataEngine::Vector3 offsetTable[kNumCorner] = {
	    {kWidth / 2.0f,  -kHeight / 2.0f, 0},
        {-kWidth / 2.0f, -kHeight / 2.0f, 0},
        {kWidth / 2.0f,  kHeight / 2.0f,  0},
        {-kWidth / 2.0f, kHeight / 2.0f,  0}
    };
	return center + offsetTable[static_cast<uint32_t>(corner)];
}

void Enemy::MapCollisionUp(CollisionMapInfo& info) {
	if (info.move.y <= 0) {
		return;
	}
	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}
	bool hit = false;
	MapChipField::IndexSet indexSetImpactedBlock = {UINT32_MAX, UINT32_MAX};
	MapChipField::IndexSet indexSetLeftTop = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSetLeftTop.xIndex, indexSetLeftTop.yIndex) == MapChipType::kBlock) {
		hit = true;
		indexSetImpactedBlock = indexSetLeftTop;
	}
	MapChipField::IndexSet indexSetRightTop = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSetRightTop.xIndex, indexSetRightTop.yIndex) == MapChipType::kBlock) {
		hit = true;
		if (indexSetImpactedBlock.xIndex == UINT32_MAX) {
			indexSetImpactedBlock = indexSetRightTop;
		}
	}
	if (hit) {
		info.isCeilingHit = true;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetImpactedBlock.xIndex, indexSetImpactedBlock.yIndex);
		float targetPlayerCenterY = blockRect.bottom - kHeight / 2.0f;
		info.move.y = targetPlayerCenterY - worldTransform_.translation_.y;
	}
}

void Enemy::MapCollisionDown(CollisionMapInfo& info) {
	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}
	bool hit = false;
	MapChipField::IndexSet indexSetImpactedBlock = {UINT32_MAX, UINT32_MAX};
	float deepestPenetrationY = -FLT_MAX;
	MapChipField::IndexSet indexSetLeftBottom = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSetLeftBottom.xIndex, indexSetLeftBottom.yIndex) == MapChipType::kBlock) {
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetLeftBottom.xIndex, indexSetLeftBottom.yIndex);
		if (positionsNew[kLeftBottom].y < blockRect.top) {
			hit = true;
			indexSetImpactedBlock = indexSetLeftBottom;
			deepestPenetrationY = positionsNew[kLeftBottom].y;
		}
	}
	MapChipField::IndexSet indexSetRightBottom = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSetRightBottom.xIndex, indexSetRightBottom.yIndex) == MapChipType::kBlock) {
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetRightBottom.xIndex, indexSetRightBottom.yIndex);
		if (positionsNew[kRightBottom].y < blockRect.top) {
			hit = true;
			if (indexSetImpactedBlock.xIndex == UINT32_MAX || positionsNew[kRightBottom].y < deepestPenetrationY) {
				indexSetImpactedBlock = indexSetRightBottom;
			}
		}
	}
	if (hit) {
		info.isLanding = true;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetImpactedBlock.xIndex, indexSetImpactedBlock.yIndex);
		float targetPlayerCenterY = blockRect.top + kHeight / 2.0f;
		info.move.y = targetPlayerCenterY - worldTransform_.translation_.y;
	}
}

void Enemy::MapCollisionRight(CollisionMapInfo& info) {
	if (info.move.x <= 0) {
		return;
	}
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + info.move;
	Vector3 rightTopCheck = centerNew + Vector3{kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 rightBottomCheck = centerNew + Vector3{kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	MapChipField::IndexSet indexSetTop = mapChipField_->GetMapChipIndexSetByPosition(rightTopCheck);
	MapChipField::IndexSet indexSetBottom = mapChipField_->GetMapChipIndexSetByPosition(rightBottomCheck);
	if ((mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ||
	    (mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock)) {
		info.isWallContact = true;
		MapChipField::IndexSet indexSet = (mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ? indexSetTop : indexSetBottom;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = blockRect.left - kWidth / 2.0f - worldTransform_.translation_.x;
	}
}

void Enemy::MapCollisionLeft(CollisionMapInfo& info) {
	if (info.move.x >= 0) {
		return;
	}
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + info.move;
	Vector3 leftTopCheck = centerNew + Vector3{-kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 leftBottomCheck = centerNew + Vector3{-kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	MapChipField::IndexSet indexSetTop = mapChipField_->GetMapChipIndexSetByPosition(leftTopCheck);
	MapChipField::IndexSet indexSetBottom = mapChipField_->GetMapChipIndexSetByPosition(leftBottomCheck);
	if ((mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ||
	    (mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock)) {
		info.isWallContact = true;
		MapChipField::IndexSet indexSet = (mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ? indexSetTop : indexSetBottom;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = blockRect.right + kWidth / 2.0f - worldTransform_.translation_.x;
	}
}

void Enemy::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
#ifdef _DEBUG
	// NULLポインタチェック
	assert(model);
#endif // _DEBUG

	// 因数として受け取ったデータをメンバ変数に記録
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	// 座標を元に行列の更新を行う
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	// 速度を設定する
	velocity_ = {-kWorkSpeed, 0.0f, 0.0f};

	workTimer_ = 0.0f;
}

void Enemy::Update() {

	// --- 1. 重力と移動 ---
	if (onGround_) {
		// 着地している場合、左右の速度を維持
		velocity_.x = (lrDirection_ == LRDirection::kLeft) ? -kWorkSpeed : kWorkSpeed;
	} else {
		// 空中にいる場合、重力を加算
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::fmaxf(velocity_.y, -kLimitFallSpeed);
	}

	// --- 2. 当たり判定と移動量の補正 ---
	// X軸
	CollisionMapInfo infoX{};
	infoX.move = {velocity_.x, 0.0f, 0.0f};
	MapCollisionRight(infoX);
	MapCollisionLeft(infoX);
	if (infoX.isWallContact) { // 壁に当たったら向きを反転
		// 現在の向きと逆の向きを新しい向きとする
		LRDirection newDirection = (lrDirection_ == LRDirection::kLeft) ? LRDirection::kRight : LRDirection::kLeft;
		// 向きが実際に変わる場合のみ、旋回アニメーションを開始する
		if (lrDirection_ != newDirection) {
			lrDirection_ = newDirection;
			// 旋回アニメーションの準備
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = 0.0f;
		}
	}
	worldTransform_.translation_.x += infoX.move.x;

	// Y軸
	CollisionMapInfo infoY{};
	infoY.move = {0.0f, velocity_.y, 0.0f};
	MapCollisionUp(infoY);
	MapCollisionDown(infoY);
	worldTransform_.translation_.y += infoY.move.y;
	if (infoY.isLanding) {
		onGround_ = true;
		velocity_.y = 0;
	} else {
		onGround_ = false;
	}
	if (infoY.isCeilingHit) {
		velocity_.y = 0;
	}

	// --- 3. アニメーションと向きの更新 ---
	workTimer_ += 1.0f / 60.0f;
	float timeInCycle = std::fmod(workTimer_, kWalkMotionTime);
	float progress = timeInCycle / kWalkMotionTime;
	float sinArgument = progress * 2.0f * std::numbers::pi_v<float>;
	float t = (std::sin(sinArgument) + 1.0f) / 2.0f;
	worldTransform_.rotation_.x = (1.0f - t) * kWalkMotionAngleStart + t * kWalkMotionAngleEnd;

	// 向きの更新
	if (turnTimer_ < 1.0f) {
		// kTimeTurnで指定した秒数でタイマーが1.0fになるように増加量を調整 (60FPS想定)
		turnTimer_ += 1.0f / (60.0f * kTimeTurn);
		turnTimer_ = std::fminf(turnTimer_, 1.0f); // 1.0fを超えないようにする

		float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> * 0.5f, // 右向き (90度)
		    std::numbers::pi_v<float> * 1.5f  // 左向き (270度)
		};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		// EaseInOutを使い、滑らかな緩急のある回転をさせる
		worldTransform_.rotation_.y = Lerp(turnFirstRotationY_, destinationRotationY, (turnTimer_));
	}

	// --- 4. 行列の更新 ---
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	// 3Dモデルを描画
	model_->Draw(worldTransform_, *camera_, textureHandle_);

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();
}

KamataEngine::Vector3 Enemy::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - (kWidth / 2.0f), worldPos.y - (kHeight / 2.0f), worldPos.z - (kWidth / 2.0f)};
	aabb.max = {worldPos.x + (kWidth / 2.0f), worldPos.y + (kHeight / 2.0f), worldPos.z + (kWidth / 2.0f)};
	return aabb;
}

void Enemy::OnCollision(const Player* player) {
	// 警告を抑制するためのダミー処理
	(void)player;
	// ここに敵がプレイヤーに当たった時の処理を書く（例：HPを減らすなど）
}
