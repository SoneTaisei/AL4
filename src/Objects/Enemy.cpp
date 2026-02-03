#include "Enemy.h"
#include "Player.h"
#include "System/GameTime.h"
#include "System/MapChipField.h"
#include "Utils/Easing.h"
#include "Utils/TransformUpdater.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include "Scenes/SoundData.h"

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

	// --- 1. 壁判定 (既存の処理) ---
	Vector3 rightTopCheck = centerNew + Vector3{kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 rightBottomCheck = centerNew + Vector3{kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	MapChipField::IndexSet indexSetTop = mapChipField_->GetMapChipIndexSetByPosition(rightTopCheck);
	MapChipField::IndexSet indexSetBottom = mapChipField_->GetMapChipIndexSetByPosition(rightBottomCheck);

	if (((mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ||
	     (mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock)) &&
	    lrDirection_ == LRDirection::kRight) {

		info.isWallContact = true;
		MapChipField::IndexSet indexSet = (mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ? indexSetTop : indexSetBottom;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = blockRect.left - kWidth / 2.0f - worldTransform_.translation_.x;

		// 壁に当たったらその時点で処理終了
		return;
	}

	// --- 2. 崖判定  ---
	// 右下のさらに少し下を調べる
	Vector3 rightFloorCheck = centerNew + Vector3{kWidth / 2.0f, -kHeight / 2.0f - 0.2f, 0.0f};
	MapChipField::IndexSet indexSetFloor = mapChipField_->GetMapChipIndexSetByPosition(rightFloorCheck);

	// 足元がブロックでなければ（＝穴なら）壁と同じ扱いにする
	if (mapChipField_->GetMapChipTypeByIndex(indexSetFloor.xIndex, indexSetFloor.yIndex) != MapChipType::kBlock) {
		if (lrDirection_ == LRDirection::kRight) {
			info.isWallContact = true; // これをtrueにするとUpdate内で反転処理が走る
			info.move.x = 0.0f;        // 進ませない
		}
	}
}

void Enemy::MapCollisionLeft(CollisionMapInfo& info) {
	if (info.move.x >= 0) {
		return;
	}
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + info.move;

	// --- 1. 壁判定 (既存の処理) ---
	Vector3 leftTopCheck = centerNew + Vector3{-kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 leftBottomCheck = centerNew + Vector3{-kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};
	MapChipField::IndexSet indexSetTop = mapChipField_->GetMapChipIndexSetByPosition(leftTopCheck);
	MapChipField::IndexSet indexSetBottom = mapChipField_->GetMapChipIndexSetByPosition(leftBottomCheck);

	if (((mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ||
	     (mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock)) &&
	    lrDirection_ == LRDirection::kLeft) {

		info.isWallContact = true;
		MapChipField::IndexSet indexSet = (mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) ? indexSetTop : indexSetBottom;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = blockRect.right + kWidth / 2.0f - worldTransform_.translation_.x;

		// 壁に当たったらその時点で処理終了
		return;
	}

	// --- 2. 崖判定 ---
	// 左下のさらに少し下を調べる
	Vector3 leftFloorCheck = centerNew + Vector3{-kWidth / 2.0f, -kHeight / 2.0f - 0.2f, 0.0f};
	MapChipField::IndexSet indexSetFloor = mapChipField_->GetMapChipIndexSetByPosition(leftFloorCheck);

	// 足元がブロックでなければ（＝穴なら）壁と同じ扱いにする
	if (mapChipField_->GetMapChipTypeByIndex(indexSetFloor.xIndex, indexSetFloor.yIndex) != MapChipType::kBlock) {
		if (lrDirection_ == LRDirection::kLeft) {
			info.isWallContact = true; // これをtrueにするとUpdate内で反転処理が走る
			info.move.x = 0.0f;        // 進ませない
		}
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

	// 初期スケールを明示
	worldTransform_.scale_ = {kInitialScale, kInitialScale, kInitialScale};

	// 座標を元に行列の更新を行う
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	// 速度を設定する
	velocity_ = {-kWorkSpeed, 0.0f, 0.0f};

	workTimer_ = 0.0f;

	// 初期状態は生存
	state_ = State::kAlive;

	// オブジェクトカラー初期化（死亡時のフェードで使う）
	objectColor_.Initialize();
	color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	// death timer reset
	deathTimer_ = 0.0f;
}

void Enemy::Update() {

	// Dead なら更新しない
	if (state_ == State::kDead) {
		return;
	}

	// 死亡中のアニメーション処理
	if (state_ == State::kDying) {
		// シーケンシャル処理：まず回転、その後縮小フェード
		float totalDuration = kDeathSpinDuration + kDeathShrinkDuration;
		deathTimer_ += GameTime::GetDeltaTime();

		if (deathTimer_ < kDeathSpinDuration) {
			// 回転フェーズ：Y軸回転のみ
			worldTransform_.rotation_.y += kDeathSpinSpeed * GameTime::GetDeltaTime();
		} else {
			// 回転が終わり、小さくなる「瞬間」だけSEを鳴らす ✨
			if (deathTimer_  < kDeathSpinDuration) {
				uint32_t vHandle = KamataEngine::Audio::GetInstance()->PlayWave(SoundData::seEnemyDeath, false);
				KamataEngine::Audio::GetInstance()->SetVolume(vHandle, 1.0f); // 音量を最大に調整 🔊
			}
			// 縮小フェーズ（回転は停止）
			float shrinkElapsed = deathTimer_ - kDeathSpinDuration;
			float t = std::clamp(shrinkElapsed / kDeathShrinkDuration, 0.0f, 1.0f);
			// スケール線形補間 1.0 -> 0.0
			float scale = (1.0f - t) * kInitialScale;
			worldTransform_.scale_ = {scale, scale, scale};
		}

		// 行列更新（回転・スケールの反映）
		TransformUpdater::WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();

		// 終了判定
		if (deathTimer_ >= totalDuration) {
			state_ = State::kDead;
		}
		return;
	}

	// --- 1. 重力と移動 --- (通常の生存時処理)
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
		if (lrDirection_ != newDirection && turnTimer_ >= 1.0f) {
			lrDirection_ = newDirection;
			// 旋回アニメーションの準備
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = 0.0f;
			infoX.move.x = 0.0f;
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
	workTimer_ += GameTime::GetDeltaTime();
	float timeInCycle = std::fmod(workTimer_, kWalkMotionTime);
	float progress = timeInCycle / kWalkMotionTime;
	float sinArgument = progress * 2.0f * std::numbers::pi_v<float>;
	float r = (std::sin(sinArgument) + 1.0f) / 2.0f;
	worldTransform_.rotation_.x = (1.0f - r) * kWalkMotionAngleStart + r * kWalkMotionAngleEnd;

	// 向きの更新
	if (turnTimer_ < 1.0f) {
		turnTimer_ += 1.0f / (60.0f * kTimeTurn);
		turnTimer_ = std::fminf(turnTimer_, 1.0f);

		float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> * 0.5f, // 右向き (90度)
		    std::numbers::pi_v<float> * 1.5f  // 左向き (270度)
		};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotation_.y = Lerp(turnFirstRotationY_, destinationRotationY, (turnTimer_));
	}

	// --- 4. 行列の更新 ---
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	// Dead は描画しない
	if (state_ == State::kDead) {
		return;
	}
	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	// 3Dモデルを描画
	// 死亡フェードの際は objectColor_ を使ってアルファを反映（ObjectColor の使い方に合わせて調整してください）
	if (state_ == State::kDying) {
		// もし ObjectColor の SetColor 等が存在する場合はここで更新すること
		// objectColor_.SetColor(color_);
		model_->Draw(worldTransform_, *camera_, textureHandle_, &objectColor_);
	} else {
		model_->Draw(worldTransform_, *camera_, textureHandle_);
	}

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

// SetIsAlive の実装: false が来たら死亡演出を開始する
void Enemy::SetIsAlive(bool isAlive) {
	if (isAlive) {
		// 復活や再利用する場合
		state_ = State::kAlive;
		deathTimer_ = 0.0f;
		worldTransform_.scale_ = {kInitialScale, kInitialScale, kInitialScale};
		color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	} else {
		// 生存状態から死亡アニメーションへ移行する
		if (state_ == State::kAlive) {
			state_ = State::kDying;
			deathTimer_ = 0.0f;
			// 物理挙動を止める
			velocity_ = {0.0f, 0.0f, 0.0f};
			onGround_ = false;
		}
	}
}
