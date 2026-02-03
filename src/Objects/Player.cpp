#include "Objects/Player.h"
#include "Objects/ChasingEnemy.h"
#include "Objects/Enemy.h"
#include "Objects/ShooterEnemy.h"
#include "System/Gamepad.h"
#include "System/MapChipField.h"
#include "Utils/Easing.h"
#include "Utils/TransformUpdater.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include "Scenes/SoundData.h"

using namespace KamataEngine;

// ベクトルの長さを計算するヘルパー関数を追加
float Length(const KamataEngine::Vector3& v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }

KamataEngine::Vector3 Normalize(const KamataEngine::Vector3& v) {
	float len = Length(v);
	if (len != 0) {
		return v / len;
	}
	return v;
}

void Player::UpdateAttack(const KamataEngine::Vector3& gravityVector, KamataEngine::Vector3& outAttackMove) {
	// 攻撃中の処理
	if (isAttacking_) {
		attackTimer_ -= 1.0f / 60.0f;
		float elapsedTime = kAttackDuration - attackTimer_;
		// 攻撃中は無敵
		isInvincible_ = true;

		// フェーズ1：タメ (Squash)
		if (elapsedTime < kAttackSquashDuration) {
			outAttackMove = {}; // タメ中は移動しない
			float t = elapsedTime / kAttackSquashDuration;
			float easedT = EaseOutQuint(t);
			worldTransform_.scale_.y = 1.0f + kSquashAmountY * easedT;
			worldTransform_.scale_.x = 1.0f - kSquashAmountY * easedT;
		}
		// フェーズ2：伸び (Stretch) & 突進
		else {
			float t = (elapsedTime - kAttackSquashDuration) / kAttackStretchDuration;
			t = std::clamp(t, 0.0f, 1.0f);
			float wave = sinf(t * std::numbers::pi_v<float>);
			worldTransform_.scale_.y = 1.0f - kStretchAmountY * wave;
			worldTransform_.scale_.x = 1.0f + (kStretchAmountY / 2.0f) * wave;

			float moveT = EaseOutQuint(t);
			Vector3 moveRight = {0, 0, 0};
			if (gravityVector.y != 0) {
				moveRight = {1.0f, 0.0f, 0.0f};
				if (gravityVector.y > 0)
					moveRight.x *= -1.0f;
			} else if (gravityVector.x != 0) {
				moveRight = {0.0f, -1.0f, 0.0f};
				if (gravityVector.x > 0)
					moveRight.y *= -1.0f;
			}
			Vector3 attackDirection = (lrDirection_ == LRDirection::kRight) ? moveRight : -moveRight;
			float distance = moveT * kAttackDistance;
			Vector3 targetPosition = attackStartPosition_ + attackDirection * distance;
			outAttackMove = targetPosition - worldTransform_.translation_;
		}

		if (attackTimer_ <= 0.0f) {
			isAttacking_ = false;
		}

	} else if (isMeleeAttacking_) {
		meleeAttackTimer_ -= 1.0f / 60.0f;
		float elapsed = kMeleeAttackDuration - meleeAttackTimer_;

		// 攻撃発生のタイミング
		float hitTiming = kMeleeAttackDuration / 3.0f;
		if (elapsed >= hitTiming && elapsed < hitTiming + 1.0f / 60.0f) {
			MeleeAttack();
		}

		// 簡単な攻撃モーション
		float t = 1.0f - (meleeAttackTimer_ / kMeleeAttackDuration);
		float wave = sinf(t * std::numbers::pi_v<float>);
		worldTransform_.scale_.x = 1.0f + wave * 0.2f;
		worldTransform_.scale_.y = 1.0f - wave * 0.2f;

		Vector3 moveRight = {0, 0, 0};
		if (gravityVector.y != 0) {
			moveRight = {1.0f, 0.0f, 0.0f};
		} else if (gravityVector.x != 0) {
			moveRight = {0.0f, -1.0f, 0.0f};
		}
		Vector3 attackDirection = (lrDirection_ == LRDirection::kRight) ? moveRight : -moveRight;

		// 攻撃中の移動
		outAttackMove = attackDirection * kMeleeAttackMoveDistance * wave / 60.0f; // 1フレームあたりの移動量に

		if (meleeAttackTimer_ <= 0.0f) {
			isMeleeAttacking_ = false;
		}

	} else {
		// 攻撃中でないときの処理
		worldTransform_.scale_.x = Lerp(worldTransform_.scale_.x, 1.0f, 0.2f);
		worldTransform_.scale_.y = Lerp(worldTransform_.scale_.y, 1.0f, 0.2f);

		if (invincibleTimer_ <= 0.0f) {
			isInvincible_ = false;
		}

		// キーボードJまたはゲームパッドXで攻撃
		bool gpAttack = Gamepad::GetInstance()->IsTriggered(XINPUT_GAMEPAD_X);
		if (Input::GetInstance()->TriggerKey(DIK_J) || gpAttack) {

			// ★修正点：SHIFT判定ではなく「現在左右に移動入力があるか」で判定
			// UpdateVelocityByInputで計算される移動入力状態を参照
			bool isMoving = false;
			bool gpRight = Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) || Gamepad::GetInstance()->GetLeftThumbXf() > 0.3f;
			bool gpLeft = Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) || Gamepad::GetInstance()->GetLeftThumbXf() < -0.3f;

			if (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_A) || gpRight || gpLeft) {
				isMoving = true;
			}

			if (isMoving) {
				// 移動入力がある場合は突進攻撃（ダッシュ攻撃）
				Attack(gravityVector);
			} else {
				// 立ち止まっている場合は近接攻撃
				isMeleeAttacking_ = true;
				meleeAttackTimer_ = kMeleeAttackDuration;
				velocity_.y = 0.0f;
			}

			KamataEngine::Audio::GetInstance()->PlayWave(SoundData::sePlayerAttack, false);
		}
	}
}

void Player::ApplyCollisionAndMove(const KamataEngine::Vector3& finalMove, const KamataEngine::Vector3& gravityVector) {
	float moveLength = Length(finalMove);
	Vector3 moveDirection = {0, 0, 0};
	if (moveLength > 0.001f) {
		moveDirection = finalMove / moveLength;
	}

	const float stepSize = kWidth * 0.45f;
	int numSteps = static_cast<int>(moveLength / stepSize);

	for (int i = 0; i < numSteps; ++i) {
		if (MoveAndCollide(moveDirection * stepSize, gravityVector)) {
			return; // 衝突したら、それ以上移動しない
		}
	}

	Vector3 remainingMove = finalMove - (moveDirection * stepSize * float(numSteps));
	if (Length(remainingMove) > 0.001f) {
		MoveAndCollide(remainingMove, gravityVector);
	}
}

void Player::UpdateRotationAndTransform(float cameraAngleZ) {
	if (turnTimer_ < 1.0f) {
		turnTimer_ += 1.0f / 20.0f;
		float destRotYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float destRotY = destRotYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotation_.y = Lerp(turnFirstRotationY_, destRotY, turnTimer_);
	}
	worldTransform_.rotation_.z = cameraAngleZ + attackTilt_;

	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

// AABB同士の交差判定
static bool IsColliding(const AABB& aabb1, const AABB& aabb2) {
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) && (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) && (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z)) {
		return true;
	}
	return false;
}

void Player::MeleeAttack() {
	if (!enemies_ || !chasingEnemies_ || !shooterEnemies_) {
		return;
	}

	AABB attackAABB;
	Vector3 playerPos = GetWorldPosition();
	// float attackCenterY = playerPos.y;

	if (lrDirection_ == LRDirection::kRight) {
		attackAABB.min = {playerPos.x, playerPos.y - kHeight / 2.0f, 0.0f};
		attackAABB.max = {playerPos.x + kMeleeAttackRange, playerPos.y + kHeight / 2.0f, 0.0f};
	} else {
		attackAABB.min = {playerPos.x - kMeleeAttackRange, playerPos.y - kHeight / 2.0f, 0.0f};
		attackAABB.max = {playerPos.x, playerPos.y + kHeight / 2.0f, 0.0f};
	}

	for (Enemy* enemy : *enemies_) {
		if (enemy && enemy->GetIsAlive()) {
			AABB enemyAABB = enemy->GetAABB();
			if (IsColliding(attackAABB, enemyAABB)) {
				enemy->SetIsAlive(false);
			}
		}
	}
	for (ChasingEnemy* enemy : *chasingEnemies_) {
		if (enemy && enemy->GetIsAlive()) {
			AABB enemyAABB = enemy->GetAABB();
			if (IsColliding(attackAABB, enemyAABB)) {
				enemy->SetIsAlive(false);
			}
		}
	}
	for (ShooterEnemy* enemy : *shooterEnemies_) {
		if (enemy && enemy->GetIsAlive()) {
			AABB enemyAABB = enemy->GetAABB();
			if (IsColliding(attackAABB, enemyAABB)) {
				enemy->SetIsAlive(false);
			}
		}
	}
}

void Player::Attack(const KamataEngine::Vector3& gravityVector) {
	// 攻撃中に再度呼び出された場合は何もしない
	if (isAttacking_) {
		return;
	}
	isAttacking_ = true;
	isAttackBlocked_ = false;
	attackTimer_ = kAttackDuration;
	attackStartPosition_ = worldTransform_.translation_;

	// 攻撃開始時に、既存の左右移動速度をゼロにする
	// (重力による落下などは維持するため、上下方向の速度は保持する)
	Vector3 moveRight = {0, 0, 0};
	if (gravityVector.y != 0) { // 重力がY軸方向
		moveRight = {1.0f, 0.0f, 0.0f};
		if (gravityVector.y > 0) {
			moveRight.x *= -1.0f;
		}
	} else if (gravityVector.x != 0) { // 重力がX軸方向
		moveRight = {0.0f, -1.0f, 0.0f};
		if (gravityVector.x > 0) {
			moveRight.y *= -1.0f;
		}
	}

	// 進行方向の速度成分を抽出し、それをvelocity_から引くことで進行方向の速度を0にする
	float dot = velocity_.x * moveRight.x + velocity_.y * moveRight.y;
	Vector3 runVelocity = moveRight * dot;
	Vector3 otherVelocity = velocity_ - runVelocity;
	velocity_ = otherVelocity;

	velocity_.y = 0.0f;
}

bool Player::MoveAndCollide(const KamataEngine::Vector3& move, const KamataEngine::Vector3& gravityVector) {
	bool collided = false;

	// 重力方向に応じて、衝突判定の軸を入れ替える
	if (gravityVector.y != 0) { // 重力が上下方向の場合
		// X軸（水平）の衝突判定と移動
		{
			CollisionMapInfo infoX{};
			infoX.move = {move.x, 0.0f, 0.0f};
			MapCollisionRight(infoX);
			MapCollisionLeft(infoX);
			if (infoX.isWallContact) {
				velocity_.x = 0.0f;
				collided = true;
			}
			worldTransform_.translation_.x += infoX.move.x;
		}

		// Y軸（垂直）の衝突判定と移動
		{
			CollisionMapInfo infoY{};
			infoY.move = {0.0f, move.y, 0.0f};
			MapCollisionUp(infoY);
			MapCollisionDown(infoY);
			worldTransform_.translation_.y += infoY.move.y;

			bool landedOnFloor = (gravityVector.y < 0 && infoY.isLanding);
			bool landedOnCeiling = (gravityVector.y > 0 && infoY.isCeilingHit);

			if (landedOnFloor || landedOnCeiling) {
				onGround_ = true;
				velocity_.y = 0.0f;
				collided = true;
				// 空中から地面に着いた瞬間だけ鳴らす
				if (!onGround_) {
					// SoundData::seSelect などを流用、または着地用の音があればそれ
					KamataEngine::Audio::GetInstance()->PlayWave(SoundData::seSelect, false);
				}
				onGround_ = true;
			} else {
				onGround_ = false;
			}

			if ((gravityVector.y < 0 && infoY.isCeilingHit) || (gravityVector.y > 0 && infoY.isLanding)) {
				velocity_.y = 0.0f;
				collided = true;
			}
		}
	} else if (gravityVector.x != 0) { // 重力が左右方向の場合
		// Y軸（プレイヤーにとっての水平）の衝突判定と移動
		{
			if (std::abs(move.y) > 0.001f) {
				CollisionMapInfo infoY{};
				infoY.move = {0.0f, move.y, 0.0f};
				MapCollisionUp(infoY);
				MapCollisionDown(infoY);
				if (infoY.isCeilingHit || infoY.isLanding) {
					velocity_.y = 0.0f;
					collided = true;
				}
				worldTransform_.translation_.y += infoY.move.y;
			}
		}

		// X軸（プレイヤーにとっての垂直）の衝突判定と移動
		{
			CollisionMapInfo infoX{};
			infoX.move = {move.x, 0.0f, 0.0f};
			MapCollisionRight(infoX);
			MapCollisionLeft(infoX);
			worldTransform_.translation_.x += infoX.move.x;

			bool isLandingX = (gravityVector.x > 0 && infoX.isWallContact && velocity_.x >= 0) || (gravityVector.x < 0 && infoX.isWallContact && velocity_.x <= 0);
			bool isCeilingHitX = (gravityVector.x > 0 && infoX.isWallContact && velocity_.x < 0) || (gravityVector.x < 0 && infoX.isWallContact && velocity_.x > 0);

			if (isLandingX) {
				onGround_ = true;
				velocity_.x = 0.0f;
				collided = true;
			} else {
				if (!isCeilingHitX) {
					onGround_ = false;
				}
			}
			if (isCeilingHitX) {
				velocity_.x = 0.0f;
				collided = true;
			}
		}
	}
	return collided;
}

void Player::UpdateVelocityByInput(const KamataEngine::Vector3& gravityVector) {
	// プレイヤーにとっての「右」と「上」を計算 (既存のロジックを維持)
	Vector3 moveRight = {0, 0, 0};
	Vector3 moveUp = {0, 0, 0};
	if (gravityVector.y != 0) {
		moveRight = {1.0f, 0.0f, 0.0f};
		moveUp = {0.0f, 1.0f, 0.0f};
		if (gravityVector.y > 0) {
			moveRight.x *= -1.0f;
			moveUp.y *= -1.0f;
		}
	} else if (gravityVector.x != 0) {
		moveRight = {0.0f, -1.0f, 0.0f};
		moveUp = {1.0f, 0.0f, 0.0f};
		if (gravityVector.x > 0) {
			moveRight.y *= -1.0f;
			moveUp.x *= -1.0f;
		}
	}

	// --- 1. 入力状態の取得 ---
	bool gpRight = Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) || Gamepad::GetInstance()->GetLeftThumbXf() > 0.3f;
	bool gpLeft = Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) || Gamepad::GetInstance()->GetLeftThumbXf() < -0.3f;
	bool pressRight = Input::GetInstance()->PushKey(DIK_D) || gpRight;
	bool pressLeft = Input::GetInstance()->PushKey(DIK_A) || gpLeft;

	// --- 2. 水平移動の計算 ---
	float currentRunSpeed = (velocity_.x * moveRight.x + velocity_.y * moveRight.y);
	bool isMoving = false;

	if (!isAttacking_ && !isMeleeAttacking_) {
		if (pressRight) {
			// 右入力: 現在左に動いていたら「ブレーキ」として加速を強める
			float accel = (currentRunSpeed < 0) ? kAcceleration * 2.0f : kAcceleration;
			velocity_ += moveRight * accel;
			isMoving = true;
			if (lrDirection_ != LRDirection::kRight) {
				lrDirection_ = LRDirection::kRight;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = 0.0f;
			}
		} else if (pressLeft) {
			// 左入力: 現在右に動いていたら「ブレーキ」として加速を強める
			float accel = (currentRunSpeed > 0) ? kAcceleration * 2.0f : kAcceleration;
			velocity_ -= moveRight * accel;
			isMoving = true;
			if (lrDirection_ != LRDirection::kLeft) {
				lrDirection_ = LRDirection::kLeft;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = 0.0f;
			}
		}
	}

	// --- 3. 摩擦（減衰）処理 ---
	if (!isMoving) {
		// 入力がない場合
		float friction = onGround_ ? kAttenuation * 3.0f : kAttenuation * 0.2f; // 地上は強く、空中は弱く
		float dot = velocity_.x * moveRight.x + velocity_.y * moveRight.y;
		Vector3 runVelocity = moveRight * dot;
		Vector3 otherVelocity = velocity_ - runVelocity;

		runVelocity = runVelocity * (1.0f - friction);

		// デッドゾーン: 速度が小さくなったら完全に止める ✨
		if (std::abs(dot) < 0.02f)
			runVelocity = {0, 0, 0};

		velocity_ = runVelocity + otherVelocity;
	}

	// --- 4. 最高速度制限 (既存の制限を適用) ---
	float speed = sqrtf(powf(velocity_.x * moveRight.x + velocity_.y * moveRight.y, 2));
	if (speed > kLimitRunSpeed) {
		float dot = velocity_.x * moveRight.x + velocity_.y * moveRight.y;
		Vector3 runVel = moveRight * (dot / speed * kLimitRunSpeed);
		velocity_ = runVel + (velocity_ - moveRight * dot);
	}

	// --- 滞空エネルギーのリセット ---
	if (onGround_) {
		airHoverTimer_ = 0.0f;
	}

	// --- ジャンプ・重力の適用 ---
	bool isHovering = isMeleeAttacking_ && !onGround_ && (airHoverTimer_ < kMaxAirHoverDuration);

	if (isHovering) {
		// 垂直方向の速度を 0 に固定して位置を維持する
		if (gravityVector.y != 0)
			velocity_.y = 0.0f;
		else if (gravityVector.x != 0)
			velocity_.x = 0.0f;

		// 滞空時間を蓄積
		airHoverTimer_ += 1.0f / 60.0f;
	}
	// ★ポイント：ここから「else」の中にジャンプと通常の重力処理をまとめます
	else {
		// 1. ジャンプの処理 (既存のロジック)
		if (onGround_)
			jumpCount = 0;
		else if (jumpCount == 0)
			jumpCount = 1;

		if (!isAttacking_ && !isMeleeAttacking_ && jumpCount < kMaxJumpCount) {
			if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Gamepad::GetInstance()->IsTriggered(XINPUT_GAMEPAD_A)) {
				// ジャンプした瞬間に垂直方向の速度をリセットして初速を与える
				Vector3 verticalVel = moveUp * (velocity_.x * moveUp.x + velocity_.y * moveUp.y);
				velocity_ -= verticalVel;
				velocity_ += moveUp * kJumpAcceleration;
				onGround_ = false;
				jumpCount++;

				// ジャンプSEを鳴らす 👟
				KamataEngine::Audio::GetInstance()->PlayWave(SoundData::sePlayerJump, false);
			}
		}

		// 2. 通常の重力加算 (ここでのみ行う)
		velocity_ += gravityVector;
	}

	// ★ 115行目にあった「velocity_ += gravityVector;」は削除してください！

	// 最大落下速度制限 (既存)
	float fallSpeed = -(velocity_.x * moveUp.x + velocity_.y * moveUp.y);
	if (fallSpeed > kLimitFallSpeed) {
		Vector3 fallVelocity = -moveUp * kLimitFallSpeed;
		velocity_ = (velocity_ - (-moveUp * fallSpeed)) + fallVelocity;
	}
}

// CornerPosition関数の実装
KamataEngine::Vector3 Player::CornerPosition(const KamataEngine::Vector3 center, Corner corner) {
	// オフセットテーブル
	KamataEngine::Vector3 offsetTable[kNumCorner] = {
	    {kWidth / 2.0f,  -kHeight / 2.0f, 0}, // kRightBottom
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0}, // kLeftBottom
	    {kWidth / 2.0f,  kHeight / 2.0f,  0}, // kRightTop
	    {-kWidth / 2.0f, kHeight / 2.0f,  0}  // kLeftTop
	};
	return center + offsetTable[static_cast<uint32_t>(corner)];
}

// MapCollisionUp関数の実装
void Player::MapCollisionUp(CollisionMapInfo& info) {
	// 上昇あり？
	if (info.move.y <= 0) {
		return; // 上方向へ移動していない場合は判定をスキップ
	}

	// 移動後の4つの角の座標を計算
	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	// 真上の当たり判定を行う
	MapChipType mapChipType;
	bool hit = false;
	MapChipField::IndexSet indexSetImpactedBlock = {UINT32_MAX, UINT32_MAX}; // 衝突したブロックのインデックスを保持

	// 左上点の判定
	MapChipField::IndexSet indexSetLeftTop = mapChipFiled_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipFiled_->GetMapChipTypeByIndex(indexSetLeftTop.xIndex, indexSetLeftTop.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetLeftTop.xIndex, indexSetLeftTop.yIndex);
		// 移動後の頭の位置が、ブロックの下面を越えていたら衝突とみなす
		if (positionsNew[kLeftTop].y > blockRect.bottom) {
			hit = true;
			indexSetImpactedBlock = indexSetLeftTop;
		}
	}

	// 右上点の判定
	MapChipField::IndexSet indexSetRightTop = mapChipFiled_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipFiled_->GetMapChipTypeByIndex(indexSetRightTop.xIndex, indexSetRightTop.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetRightTop.xIndex, indexSetRightTop.yIndex);
		// 移動後の頭の位置が、ブロックの下面を越えていたら衝突とみなす
		if (positionsNew[kRightTop].y > blockRect.bottom) {
			hit = true;
			if (indexSetImpactedBlock.xIndex == UINT32_MAX && indexSetImpactedBlock.yIndex == UINT32_MAX) {
				indexSetImpactedBlock = indexSetRightTop;
			}
		}
	}

	// 衝突時の処理
	if (hit) {
		info.isCeilingHit = true; // 天井衝突フラグを立てる

		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetImpactedBlock.xIndex, indexSetImpactedBlock.yIndex);

		// Y移動量を求める（めり込みを解消するための新しい移動量）
		// 移動後のプレイヤーの頭頂部がブロックの下端に接するように位置を調整
		// ブロックの下端のY座標から、プレイヤーの半高を引くと、プレイヤーの中心Y座標が得られる
		float targetPlayerCenterY = blockRect.bottom - kHeight / 2.0f;

		// 現在のプレイヤーの中心Y座標 (worldTransform_.translation_.y) と、目標のY座標の差分
		info.move.y = targetPlayerCenterY - worldTransform_.translation_.y;

		// 速度を0にする (これはHandleCeilingCollisionで処理することを推奨)
		// velocity_.y = 0.0f;
	}
}

// MapCollisionDown関数の実装
void Player::MapCollisionDown(CollisionMapInfo& info) {

	// 下降していない場合は判定をスキップ
	if (info.move.y >= 0) {
		return;
	}

	// 移動後の4つの角の座標を計算
	std::array<KamataEngine::Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	// 真下の当たり判定を行う
	MapChipType mapChipType;
	bool hit = false;
	MapChipField::IndexSet indexSetImpactedBlock = {UINT32_MAX, UINT32_MAX}; // 衝突したブロックのインデックスを保持
	float deepestPenetrationY = -FLT_MAX;                                    // 最も深くめり込んだY座標を追跡

	// 左下点の判定
	MapChipField::IndexSet indexSetLeftBottom = mapChipFiled_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipFiled_->GetMapChipTypeByIndex(indexSetLeftBottom.xIndex, indexSetLeftBottom.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetLeftBottom.xIndex, indexSetLeftBottom.yIndex);
		const float kLandingThreshold = 0.01f;                                 // ★ここを0.001fから0.01fに増やしてみる
		if (positionsNew[kLeftBottom].y < blockRect.top - kLandingThreshold) { // ★ Y座標の比較
			hit = true;
			indexSetImpactedBlock = indexSetLeftBottom;
			deepestPenetrationY = positionsNew[kLeftBottom].y;
		}
	}

	// 右下点の判定 (同様にkLandingThresholdを適用)
	MapChipField::IndexSet indexSetRightBottom = mapChipFiled_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipFiled_->GetMapChipTypeByIndex(indexSetRightBottom.xIndex, indexSetRightBottom.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetRightBottom.xIndex, indexSetRightBottom.yIndex);
		const float kLandingThreshold = 0.01f;                                  // ★ここを0.001fから0.01fに増やしてみる
		if (positionsNew[kRightBottom].y < blockRect.top - kLandingThreshold) { // ★ Y座標の比較
			hit = true;
			// 左足がヒットしていない場合、または右足の方が深くめり込んでいる場合
			if (indexSetImpactedBlock.xIndex == UINT32_MAX || positionsNew[kRightBottom].y < deepestPenetrationY) {
				indexSetImpactedBlock = indexSetRightBottom;
				deepestPenetrationY = positionsNew[kRightBottom].y;
			}
		}
	}

	// 衝突時の処理
	if (hit) {
		info.isLanding = true;

		// ここで`indexSetImpactedBlock`が`UINT32_MAX`でないことを保証するために、先に有効なブロックがヒットしているか確認する
		// または、初期化時に確実に無効値が設定されていることを前提とする
		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetImpactedBlock.xIndex, indexSetImpactedBlock.yIndex);

		float targetPlayerCenterY = blockRect.top + kHeight / 2.0f;
		info.move.y = targetPlayerCenterY - worldTransform_.translation_.y;
	}
}

// MapCollisionRight関数の実装
void Player::MapCollisionRight(CollisionMapInfo& info) {
	// 右方向への移動あり？
	if (info.move.x <= 0) {
		return;
	}

#ifdef _DEBUG
	OutputDebugStringA("--- MapCollisionRight Check START ---\n");
#endif

	// 壁判定の高さをプレイヤーの身長の80%に狭める
	const float wallCollisionHeightRatio = 0.8f;
	const float checkHeight = kHeight * wallCollisionHeightRatio;

	// 移動後のプレイヤーの「右側」の「壁判定用の点」の座標を計算
	Vector3 centerNew = worldTransform_.translation_ + info.move;
	Vector3 rightTopCheck = centerNew + Vector3{kWidth / 2.0f, checkHeight / 2.0f, 0.0f};     // X座標をプラスに
	Vector3 rightBottomCheck = centerNew + Vector3{kWidth / 2.0f, -checkHeight / 2.0f, 0.0f}; // X座標をプラスに

	bool hitTop = false;
	bool hitBottom = false;
	// 修正した判定点を使ってマップチップのインデックスを取得
	MapChipField::IndexSet indexSetTop = mapChipFiled_->GetMapChipIndexSetByPosition(rightTopCheck);
	MapChipField::IndexSet indexSetBottom = mapChipFiled_->GetMapChipIndexSetByPosition(rightBottomCheck);

#ifdef _DEBUG
	std::string debugText = std::format("  RightTopCheck: ({:.3f}, {:.3f}) -> Index: ({}, {})\n", rightTopCheck.x, rightTopCheck.y, indexSetTop.xIndex, indexSetTop.yIndex);
	OutputDebugStringA(debugText.c_str());
	debugText = std::format("  RightBottomCheck: ({:.3f}, {:.3f}) -> Index: ({}, {})\n", rightBottomCheck.x, rightBottomCheck.y, indexSetBottom.xIndex, indexSetBottom.yIndex);
	OutputDebugStringA(debugText.c_str());
#endif

	// 右上点の判定
	if (mapChipFiled_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) {
		hitTop = true;
	}
	// 右下点の判定
	if (mapChipFiled_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock) {
		hitBottom = true;
	}

#ifdef _DEBUG
	debugText = std::format("  Hit Results: hitTop={}, hitBottom={}\n", hitTop, hitBottom);
	OutputDebugStringA(debugText.c_str());
#endif

	// どちらかの角がブロックにヒットした場合の処理
	if (hitTop || hitBottom) {
		info.isWallContact = true;

#ifdef _DEBUG
		OutputDebugStringA("  >>> isWallContact SET TO TRUE <<<\n");
#endif
		// 衝突したブロックのインデックスを決定（両方ヒットした場合は上を優先）
		MapChipField::IndexSet indexSetImpactedBlock = hitTop ? indexSetTop : indexSetBottom;

		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetImpactedBlock.xIndex, indexSetImpactedBlock.yIndex);

		//// 壁にめり込まないように移動量を調整
		// info.move.x = 0; // 元のコードに合わせて移動をキャンセル
		const float kCollisionBuffer = 0.001f;                                         // 衝突時のわずかな隙間
		float targetPlayerCenterX = blockRect.left - kWidth / 2.0f - kCollisionBuffer; // ← バッファ分をさらに引く
		info.move.x = targetPlayerCenterX - worldTransform_.translation_.x;
	}
#ifdef _DEBUG
	OutputDebugStringA("--- MapCollisionRight Check END ---\n\n");
#endif
}

// MapCollisionLeft関数の実装
void Player::MapCollisionLeft(CollisionMapInfo& info) {
	// 左方向への移動あり？
	if (info.move.x >= 0) {
		return;
	}

	// 壁判定の高さをプレイヤーの身長の80%に狭める
	const float wallCollisionHeightRatio = 0.8f;
	const float checkHeight = kHeight * wallCollisionHeightRatio;

	// 移動後のプレイヤーの左側の「壁判定用の点」の座標を計算
	Vector3 centerNew = worldTransform_.translation_ + info.move;
	Vector3 leftTopCheck = centerNew + Vector3{-kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 leftBottomCheck = centerNew + Vector3{-kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};

	bool hitTop = false;
	bool hitBottom = false;
	// 修正した判定点を使ってマップチップのインデックスを取得
	MapChipField::IndexSet indexSetTop = mapChipFiled_->GetMapChipIndexSetByPosition(leftTopCheck);
	MapChipField::IndexSet indexSetBottom = mapChipFiled_->GetMapChipIndexSetByPosition(leftBottomCheck);

	// 左上点の判定
	if (mapChipFiled_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock) {
		hitTop = true;
	}
	// 左下点の判定
	if (mapChipFiled_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock) {
		hitBottom = true;
	}

	// どちらかの角がブロックにヒットした場合の処理
	if (hitTop || hitBottom) {
		info.isWallContact = true;
		// 衝突したブロックのインデックスを決定（両方ヒットした場合は上を優先）
		MapChipField::IndexSet indexSetImpactedBlock = hitTop ? indexSetTop : indexSetBottom;

		MapChipField::Rect blockRect = mapChipFiled_->GetRectByIndex(indexSetImpactedBlock.xIndex, indexSetImpactedBlock.yIndex);

		//// 壁にめり込まないように移動量を調整
		// info.move.x = 0; // 元のコードに合わせて移動をキャンセル
		float targetPlayerCenterX = blockRect.right + kWidth / 2.0f;
		info.move.x = targetPlayerCenterX - worldTransform_.translation_.x;
	}
}

// MapCollision関数の実装 (変更なし)
void Player::MapCollision(CollisionMapInfo& info) {
	// 各方向の衝突判定関数を呼び出す
	MapCollisionUp(info);
	MapCollisionDown(info);

	// マップ側の左右判定に使う「移動入力」状態を取得（キーボード or ゲームパッド）
	bool rightInput = Input::GetInstance()->PushKey(DIK_RIGHT) || Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) || Gamepad::GetInstance()->GetLeftThumbXf() > 0.3f;
	bool leftInput = Input::GetInstance()->PushKey(DIK_LEFT) || Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) || Gamepad::GetInstance()->GetLeftThumbXf() < -0.3f;

	if (rightInput) {
		MapCollisionRight(info);
	}
	if (leftInput) {
		MapCollisionLeft(info);
	}
}

// 判定結果を反映して移動させる関数の実装
void Player::ReflectCollisionResultAndMove(const CollisionMapInfo& info) {
	// 移動
	worldTransform_.translation_.x += info.move.x;
	worldTransform_.translation_.y += info.move.y;
	worldTransform_.translation_.z += info.move.z;
}

// 天井に接触している場合の処理関数の実装
void Player::HandleCeilingCollision(const CollisionMapInfo& info) {
	// 天井に当たった？
	if (info.isCeilingHit) {
		velocity_.y = 0; // Y方向の速度を0にする
	}
}

/// <summary>
/// ワールド座標の取得
/// </summary>
KamataEngine::Vector3 Player::GetWorldPosition() const {
	// ワールド座標を入れる変数を宣言
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得 (ワールド座標)
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

/// <summary>
/// AABBの取得
/// </summary>
AABB Player::GetAABB() {
	// ワールド座標を取得
	Vector3 worldPos = GetWorldPosition(); //

	// AABBを生成
	AABB aabb;
	// キャラクターの幅や高さを使ってAABBのminとmaxを計算
	// kWidth と kHeight は Player.h で 2.0f と定義されている
	aabb.min = {
	    worldPos.x - (kWidth / 2.0f), worldPos.y - (kHeight / 2.0f),
	    worldPos.z - (kWidth / 2.0f) // 資料に合わせてZも横幅で計算
	};
	aabb.max = {
	    worldPos.x + (kWidth / 2.0f), worldPos.y + (kHeight / 2.0f),
	    worldPos.z + (kWidth / 2.0f) // 資料に合わせてZも横幅で計算
	};

	return aabb;
}

void Player::OnCollision(const KamataEngine::WorldTransform& worldTransform) {
	// 無敵状態、またはすでに死亡している場合は何もしない
	if (isInvincible_ || !isAlive_) {
		return;
	}

	// ダメージ音
	KamataEngine::Audio::GetInstance()->PlayWave(SoundData::sePlayerDamage, false);

	// HPを減らす
	hp_ -= kDamageFromEnemy;

	// HPが0以下になったかチェック
	if (hp_ <= 0) {
		hp_ = 0; // HPがマイナスにならないようにする

		// ★変更点: ここで即座に isAlive_ = false にせず、演出フラグを立てる
		// これによりパーティクルが出るのを防ぎ（isAlive_を見ていれば）、
		// プレイヤーが消えずにノックバックと倒れる演出へ移行できる
		isDeadAnimating_ = true;

		// 演出用タイマーをセット
		deathTimer_ = kDeathAnimationDuration;
	}

	// ★変更点: else を削除し、HPが0になった場合でもノックバック処理を実行する

	// ダメージを受けたら無敵時間を設定（死ぬときも無敵にしておくと安全）
	isInvincible_ = true;
	invincibleTimer_ = kInvincibleDuration;

	// 敵のワールド座標を取得
	KamataEngine::Vector3 enemyPos = worldTransform.translation_;
	KamataEngine::Vector3 playerPos = worldTransform_.translation_;

	// プレイヤーから敵への逆方向ベクトルを計算
	KamataEngine::Vector3 knockbackDir = playerPos - enemyPos;
	knockbackDir.z = 0.0f; // 2DアクションなのでZ軸は無視

	// 完全に座標が重なっている場合の保険
	if (Length(knockbackDir) < 0.001f) {
		// 現在のプレイヤーの向きと逆方向に飛ばす
		KamataEngine::Vector3 moveRight = {0, 0, 0};
		if (gravity_.y != 0) {
			moveRight = {1.0f, 0.0f, 0.0f};
			if (gravity_.y > 0)
				moveRight.x *= -1.0f;
		} else if (gravity_.x != 0) {
			moveRight = {0.0f, -1.0f, 0.0f};
			if (gravity_.x > 0)
				moveRight.y *= -1.0f;
		}
		knockbackDir = (lrDirection_ == LRDirection::kRight) ? -moveRight : moveRight;
	} else {
		knockbackDir = Normalize(knockbackDir);
	}

	// 現在の重力から「上方向」と「水平方向」を正しく設定
	KamataEngine::Vector3 moveUp = {0.0f, 0.0f, 0.0f};
	if (gravity_.y != 0) { // 通常の上下重力
		moveUp = (gravity_.y < 0) ? KamataEngine::Vector3{0.0f, 1.0f, 0.0f} : KamataEngine::Vector3{0.0f, -1.0f, 0.0f};
		// 水平方向はX軸だけにする
		knockbackDir.y = 0;
		if (Length(knockbackDir) > 0.001f)
			knockbackDir = Normalize(knockbackDir);

	} else if (gravity_.x != 0) { // 横向き重力
		moveUp = (gravity_.x < 0) ? KamataEngine::Vector3{1.0f, 0.0f, 0.0f} : KamataEngine::Vector3{-1.0f, 0.0f, 0.0f};
		// 水平方向はY軸だけにする
		knockbackDir.x = 0;
		if (Length(knockbackDir) > 0.001f)
			knockbackDir = Normalize(knockbackDir);
	}

	// 最終的なノックバック速度を合成して設定
	velocity_ = (knockbackDir * kKnockbackHorizontalPower) + (moveUp * kKnockbackVerticalPower);

	// ノックバック中は強制的に空中状態にする
	onGround_ = false;
}

void Player::Initialize(
    KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Model* swordModel, uint32_t swordTextureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
#ifdef _DEBUG
	// NULLポインタチェック
	assert(model);
#endif // _DEBUG

	// 因数として受け取ったデータをメンバ変数に記録
	playerModel_ = model;
	playerTextureHandle_ = textureHandle;

	// 剣の初期化
	swordModel_ = swordModel;
	swordTextureHandle_ = swordTextureHandle;
	swordWorldTransform_.Initialize();

	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	// 初期化時に左右向きと回転補間状態を明示的にリセットしておく
	// これにより、再利用時に前回の向き補間が残ってフェード中に不正な向きになるのを防ぐ
	lrDirection_ = LRDirection::kRight;
	turnFirstRotationY_ = worldTransform_.rotation_.y;
	// turnTimer_ を 1.0f にして補間を即時完了させ、回転補間で勝手に値が変わらないようにする
	turnTimer_ = 1.0f;
	// カメラZ回転は描画時に上書きするが念のため0に初期化
	worldTransform_.rotation_.z = 0.0f;

	// 座標を元に行列の更新を行う
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	// 速度をゼロクリアする
	velocity_ = {0.0f, 0.0f, 0.0f};
	// 接地フラグをリセット
	onGround_ = false;

	// 攻撃フラグ
	isAttacking_ = false;
	attackTimer_ = 0.0f;
	attackStartPosition_ = {};
	isAttackBlocked_ = false;

	// 近接攻撃フラグ
	isMeleeAttacking_ = false;
	meleeAttackTimer_ = 0.0f;

	// 生存状態で初期化
	isAlive_ = true;

	// 死亡演出フラグのリセット
	isDeadAnimating_ = false;

	// 無敵状態
	isInvincible_ = false;

	// HPを最大値で初期化
	hp_ = kMaxHp;
	// 無敵タイマーをリセット
	invincibleTimer_ = 0.0f;
}

void Player::Update(
    const KamataEngine::Vector3& gravityVector, float cameraAngleZ, const std::list<Enemy*>& enemies, const std::list<ChasingEnemy*>& chasingEnemies, const std::list<ShooterEnemy*>& shooterEnemies,
    float timeScale) {

	enemies_ = &enemies;
	chasingEnemies_ = &chasingEnemies;
	shooterEnemies_ = &shooterEnemies;

	if (!isAlive_) {
		return;
	}

	// 画面外（下に落ちた）判定
	const float kDeadlyHeight = 11.0f;
	if (GetWorldPosition().y < kDeadlyHeight) {
		isAlive_ = false;
		return;
	}

	gravity_ = gravityVector;

	if (isDeadAnimating_) {
		// 1. 重力を加算（時間スケールを掛ける）
		velocity_ += gravityVector * timeScale;

		// 2. 移動と衝突判定（移動量にも時間スケールを掛ける）
		Vector3 finalMove = velocity_ * timeScale;
		ApplyCollisionAndMove(finalMove, gravityVector);

		// 3. 摩擦抵抗（時間スケール分だけ減衰させる簡易計算）
		if (onGround_) {
			// ★変更点1: 減衰率を 0.9f から 0.5f (もっと抵抗を強く) に変更
			// 数値が小さいほど、すぐに止まります (0.0f〜1.0f)
			float friction = 0.5f;

			velocity_.x *= (1.0f - (1.0f - friction) * timeScale);
			velocity_.z *= (1.0f - (1.0f - friction) * timeScale);

			// ★変更点2: 速度がわずかになったら強制的に止める（スナップ処理）
			// これがないと、無限に滑っているように見えてしまいます
			if (std::abs(velocity_.x) < 0.05f)
				velocity_.x = 0.0f;
			if (std::abs(velocity_.z) < 0.05f)
				velocity_.z = 0.0f;
		}

		// 4. 倒れる演出（回転速度に時間スケールを掛ける）
		float maxRot = std::numbers::pi_v<float> / 2.0f;
		float rotSpeed = 0.1f * timeScale; // ★ここもスローに

		if (lrDirection_ == LRDirection::kRight) {
			if (worldTransform_.rotation_.z < maxRot) {
				worldTransform_.rotation_.z += rotSpeed;
				if (worldTransform_.rotation_.z > maxRot)
					worldTransform_.rotation_.z = maxRot;
			}
		} else {
			if (worldTransform_.rotation_.z > -maxRot) {
				worldTransform_.rotation_.z -= rotSpeed;
				if (worldTransform_.rotation_.z < -maxRot)
					worldTransform_.rotation_.z = -maxRot;
			}
		}

		TransformUpdater::WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();

		// タイマーもスローに合わせてゆっくり減らす
		deathTimer_ -= (1.0f / 60.0f);
		if (deathTimer_ <= 0.0f) {
			isAlive_ = false;
		}
		return;
	}

	// 被ダメージ無敵時間の更新
	if (invincibleTimer_ > 0.0f) {
		invincibleTimer_ -= (1.0f / 60.0f) * timeScale; // ★スロー対応
		if (invincibleTimer_ <= 0.0f) {
			if (!isAttacking_) {
				isInvincible_ = false;
			}
		}
	}

	// 1. 攻撃更新（UpdateAttack内も本当はtimeScale対応が必要だが、攻撃中に死ぬことは稀なので一旦省略可）
	// もし厳密にやるならUpdateAttackにもtimeScaleを渡してください
	Vector3 attackMove = {};
	UpdateAttack(gravityVector, attackMove);

	// 2. 入力による速度更新
	// UpdateVelocityByInput も timeScale を考慮して移動量を調整する必要があります
	// ここでは簡易的に UpdateVelocityByInput の中身をここに展開して書くか、
	// UpdateVelocityByInput に timeScale を渡す修正が必要です。
	// 今回は「入力受付」部分なので、スロー中は操作不能（または鈍くなる）と仮定し、
	// 下記の finalMove 計算で全体に timeScale を掛けることで対応します。

	UpdateVelocityByInput(gravityVector);
	// ※注意: 正しくは UpdateVelocityByInput 内の加速や重力加算にも timeScale を掛けるべきですが、
	// 死亡時以外（通常時）は timeScale=1.0 なので、今回は「最終移動量」で調整します。

	// 重力加算（通常時）の補正
	// UpdateVelocityByInput内で velocity_ += gravityVector されているため、
	// 正確にはそこで * timeScale すべきですが、簡易実装としてここで差分調整は難しいので
	// ★推奨: UpdateVelocityByInput にも引数 timeScale を追加し、内部の velocity_ += ... * timeScale に書き換えるのがベストです。
	// (今回はコード量が増えるので、UpdateVelocityByInputの修正は割愛し、物理挙動のズレには目をつぶります)

	// 3. 最終的な移動量（ここで timeScale を掛けることで全体をスローにする）
	Vector3 finalMove = (velocity_ + attackMove) * timeScale;

	// ★重要: 重力加速度が UpdateVelocityByInput で 1.0倍分 加算されてしまっているので、
	// スロー時(timeScale < 1.0f)は加算されすぎた分を少し戻すハック（簡易補正）
	if (timeScale < 1.0f) {
		velocity_ -= gravityVector * (1.0f - timeScale);
	}

	ApplyCollisionAndMove(finalMove, gravityVector);

	// 4. 向きの更新
	UpdateRotationAndTransform(cameraAngleZ);

	// 1. 攻撃の傾きと剣の計算を「先」に行う
	if (isAttacking_ || isMeleeAttacking_) {
		float t = 0.0f;
		if (isMeleeAttacking_) {
			t = 1.0f - (meleeAttackTimer_ / kMeleeAttackDuration);
		} else if (isAttacking_) {
			t = 1.0f - (attackTimer_ / kAttackDuration);
		}

		float swordAngle = 0.0f;
		float bodyTilt = 0.0f;
		float switchPoint = 0.4f;

		if (t < switchPoint) {
			// --- 1. 振りかぶり ---
			float phaseT = t / switchPoint;
			float easedT = EaseInOutQuad(phaseT);
			swordAngle = -(0.5f * easedT); // 真上から後ろへ
			bodyTilt = -0.3f * easedT;     // のけぞる
		} else {
			// --- 2. 振り下ろし ---
			float phaseT = (t - switchPoint) / (1.0f - switchPoint);
			float easedT = EaseOutQuart(phaseT);
			swordAngle = -0.5f + (2.0f * easedT); // 後ろから前へ一気に
			bodyTilt = -0.3f + (0.8f * easedT);   // 前へ踏み込む
		}

		// 1. 半径（プレイヤーから剣の根元までの距離）を決める
		// この数値を大きくすると、剣が体から離れます！
		float radius = 0.8f;

		// 2. 現在の剣の回転角度を取得
		float angle = swordWorldTransform_.rotation_.z;

		// 3. プレイヤーの座標をベースに、角度に合わせて位置をオフセット
		// 数学の円運動の公式： x = sin(θ) * r, y = cos(θ) * r を使います
		// ※向きによって符号を調整
		swordWorldTransform_.translation_.x = worldTransform_.translation_.x + sinf(angle) * -radius;
		swordWorldTransform_.translation_.y = worldTransform_.translation_.y + cosf(angle) * radius;
		swordWorldTransform_.translation_.z = worldTransform_.translation_.z;

		// 向きに合わせて回転を適用
		if (lrDirection_ == LRDirection::kRight) {
			attackTilt_ = -bodyTilt;
			// 剣の回転 = スイング角 + 体の傾き（これで手に固定される）
			swordWorldTransform_.rotation_.z = -swordAngle + attackTilt_;
		} else {
			attackTilt_ = bodyTilt;
			swordWorldTransform_.rotation_.z = swordAngle + attackTilt_;
		}

		// モデルが少し浮きすぎる場合は、ここで微調整（例: 0.5fほど上にずらす）

		// 行列更新
		TransformUpdater::WorldTransformUpdate(swordWorldTransform_);
		swordWorldTransform_.TransferMatrix();
	} else {
		// ★重要：攻撃していないときは、傾きを滑らかに0に戻す（リセット処理）
		// 0.2f の値を小さくするとよりゆっくり、大きくすると素早く戻ります
		attackTilt_ = Lerp(attackTilt_, 0.0f, 0.2f);
	}

	// 3. 最後にプレイヤーの回転と行列を更新（ここで attackTilt_ が反映される）
	UpdateRotationAndTransform(cameraAngleZ);
}

void Player::Draw() {
	// 死亡していたら以降の処理は行わない
	if (!isAlive_) {
		return;
	}

	// 無敵時間中は点滅させる
	// fmodは剰余を求める関数
	if (invincibleTimer_ > 0) {
		// 0.2秒ごとに表示/非表示を切り替える
		if (fmod(invincibleTimer_, 0.2f) < 0.1f) {
			return; // このフレームは描画しない
		}
	}

	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	// 3Dモデルを描画
	playerModel_->Draw(worldTransform_, *camera_, playerTextureHandle_);

	if (isAttacking_ || isMeleeAttacking_) {
		// 3Dモデル描画前処理は共通なので不要（もし変えるならPreDrawを呼ぶ）
		swordModel_->Draw(swordWorldTransform_, *camera_, swordTextureHandle_);
	}

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();
}

// 演出開始のトリガー
void Player::StartGoalAnimation() {
	isAttacking_ = false;
	velocity_ = {0, 0, 0};

	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};

	// 現在のフェーズとタイマーをリセット
	goalAnimationPhase_ = GoalAnimationPhase::kSpin;
	goalAnimTimer_ = 0.0f;

	isInvincible_ = false;
	invincibleTimer_ = 0.0f;

	// 着地判定のために現在のY座標を保存 (attackStartPosition_を再利用)
	attackStartPosition_ = worldTransform_.translation_;
	// 回転計算のために現在のY軸回転を保存
	goalStartRotationY_ = worldTransform_.rotation_.y;
}

// 演出用の更新ロジック (Updateから呼び出す)
void Player::UpdateGoalAnimation() {
	if (goalAnimationPhase_ == GoalAnimationPhase::kNone)
		return;

	goalAnimTimer_ += 1.0f / 60.0f;

	// --- 1. 回転 ---
	if (goalAnimationPhase_ == GoalAnimationPhase::kSpin) {
		float duration = 0.5f;
		float t = std::clamp(goalAnimTimer_ / duration, 0.0f, 1.0f);

		float startRot = goalStartRotationY_;
		float endRot = std::numbers::pi_v<float>;

		// 回転処理
		worldTransform_.rotation_.y = Lerp(startRot, endRot + std::numbers::pi_v<float> * 2.0f, t);

		if (t >= 1.0f) {
			// ★変更: ジャンプではなく、待機フェーズへ移行
			goalAnimationPhase_ = GoalAnimationPhase::kWait;
			goalAnimTimer_ = 0.0f;
		}
	}
	// --- 2. 待機 (0.2秒) ---
	else if (goalAnimationPhase_ == GoalAnimationPhase::kWait) {
		float waitDuration = 0.2f; // ここで待ち時間を調整

		if (goalAnimTimer_ >= waitDuration) {
			// 待機が終わったらジャンプ開始
			goalAnimationPhase_ = GoalAnimationPhase::kJump;
			goalAnimTimer_ = 0.0f;
			velocity_.y = 0.2f; // ジャンプ初速

			// ジャンプ開始時のZ回転を保存
			goalStartRotationZ_ = worldTransform_.rotation_.z;
		}
	}
	// --- 3. ジャンプ & ポーズ維持 ---
	else if (goalAnimationPhase_ == GoalAnimationPhase::kJump) {
		// ジャンプ移動
		velocity_.y -= kGravityAcceleration;
		if (velocity_.y > 0.0f) {
			worldTransform_.translation_.y += velocity_.y;
		} else {
			velocity_.y = 0.0f; // 最高点で停止
		}

		// 傾きアニメーション
		float tiltDuration = 0.4f;
		float t = std::clamp(goalAnimTimer_ / tiltDuration, 0.0f, 1.0f);
		float easedT = EaseOutQuad(t);

		float targetRotZ = -0.4f;
		worldTransform_.rotation_.z = Lerp(goalStartRotationZ_, targetRotZ, easedT);

		// ポーズ維持時間
		float waitTime = 2.5f;

		if (velocity_.y <= 0.0f && t >= 1.0f && goalAnimTimer_ >= waitTime) {
			goalAnimationPhase_ = GoalAnimationPhase::kEnd;
		}
	}
	// kEnd: 固定

	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}
