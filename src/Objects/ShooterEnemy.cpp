#include "ShooterEnemy.h"
#include "Player.h"
#include "System/GameTime.h"
#include "Utils/Easing.h"
#include "Utils/TransformUpdater.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void ShooterEnemy::Initialize(Model* model, Model* projectileModel, uint32_t textureHandle, uint32_t projectileTextureHandle, Camera* camera, const Vector3& position) {
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	projectileModel_ = projectileModel;
	projectileTextureHandle_ = projectileTextureHandle;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	// --- 向きの修正 ---
	// 左に進むため、最初から左(270度)に向けておく
	worldTransform_.rotation_.y = std::numbers::pi_v<float> * 1.5f;

	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	// 移動速度は左方向
	velocity_ = {-kMoveSpeed, 0.0f, 0.0f};

	// 内部の向き管理フラグも左
	lrDirection_ = LRDirection::kLeft;

	shootTimer_ = 0.0f;

	// --- 旋回アニメーション管理変数の初期化 ---
	// 最初は旋回しなくていい（完了状態）ので 1.0f に設定
	turnTimer_ = 1.0f;
	// 念のため開始角度変数も現在の向きに合わせておく
	turnFirstRotationY_ = worldTransform_.rotation_.y;

	// --- 状態と色の初期化 ---
	state_ = State::kAlive;
	objectColor_.Initialize();
	color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	deathTimer_ = 0.0f;
}

void ShooterEnemy::MapCollisionRight(Vector3& move) {
	// --- 1. 壁判定 (既存の処理) ---
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + move;
	Vector3 rightTopCheck = centerNew + Vector3{kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 rightBottomCheck = centerNew + Vector3{kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};

	auto idxTop = mapChipField_->GetMapChipIndexSetByPosition(rightTopCheck);
	auto idxBottom = mapChipField_->GetMapChipIndexSetByPosition(rightBottomCheck);

	if ((mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock ||
	     mapChipField_->GetMapChipTypeByIndex(idxBottom.xIndex, idxBottom.yIndex) == MapChipType::kBlock) &&
	    lrDirection_ == LRDirection::kRight) {
		// 壁接触 -> 反転
		lrDirection_ = LRDirection::kLeft;
		move.x = 0.0f;

		turnFirstRotationY_ = worldTransform_.rotation_.y;
		turnTimer_ = 0.0f;
		return; // 壁に当たったらここで終了
	}

	// --- 2. 崖判定 ---
	// 「移動先の足元」をチェックする
	// 右端(kWidth/2.0f) のさらに少し下(-checkHeight / 2.0f - 0.2f) を調べる
	Vector3 rightFloorCheck = centerNew + Vector3{kWidth / 2.0f, -kHeight / 2.0f - 0.2f, 0.0f};
	auto idxFloor = mapChipField_->GetMapChipIndexSetByPosition(rightFloorCheck);

	// 足元がブロックじゃなかったら（＝穴だったら）反転
	if (mapChipField_->GetMapChipTypeByIndex(idxFloor.xIndex, idxFloor.yIndex) != MapChipType::kBlock) {
		lrDirection_ = LRDirection::kLeft;
		move.x = 0.0f;

		turnFirstRotationY_ = worldTransform_.rotation_.y;
		turnTimer_ = 0.0f;
	}
}

void ShooterEnemy::MapCollisionLeft(Vector3& move) {
	if (!mapChipField_)
		return;
	if (move.x >= 0)
		return;

	// --- 1. 壁判定 (既存の処理) ---
	const float checkHeight = kHeight * 0.8f;
	Vector3 centerNew = worldTransform_.translation_ + move;
	Vector3 leftTopCheck = centerNew + Vector3{-kWidth / 2.0f, checkHeight / 2.0f, 0.0f};
	Vector3 leftBottomCheck = centerNew + Vector3{-kWidth / 2.0f, -checkHeight / 2.0f, 0.0f};

	auto idxTop = mapChipField_->GetMapChipIndexSetByPosition(leftTopCheck);
	auto idxBottom = mapChipField_->GetMapChipIndexSetByPosition(leftBottomCheck);

	if ((mapChipField_->GetMapChipTypeByIndex(idxTop.xIndex, idxTop.yIndex) == MapChipType::kBlock ||
	     mapChipField_->GetMapChipTypeByIndex(idxBottom.xIndex, idxBottom.yIndex) == MapChipType::kBlock) &&
	    lrDirection_ == LRDirection::kLeft) {
		lrDirection_ = LRDirection::kRight;
		move.x = 0.0f;

		turnFirstRotationY_ = worldTransform_.rotation_.y;
		turnTimer_ = 0.0f;
		return; // 壁に当たったらここで終了
	}

	// --- 2. 崖判定　---
	// 「移動先の足元」をチェックする
	// 左端(-kWidth/2.0f) のさらに少し下(-kHeight / 2.0f - 0.2f) を調べる
	Vector3 leftFloorCheck = centerNew + Vector3{-kWidth / 2.0f, -kHeight / 2.0f - 0.2f, 0.0f};
	auto idxFloor = mapChipField_->GetMapChipIndexSetByPosition(leftFloorCheck);

	// 足元がブロックじゃなかったら（＝穴だったら）反転
	if (mapChipField_->GetMapChipTypeByIndex(idxFloor.xIndex, idxFloor.yIndex) != MapChipType::kBlock) {
		lrDirection_ = LRDirection::kRight;
		move.x = 0.0f;

		turnFirstRotationY_ = worldTransform_.rotation_.y;
		turnTimer_ = 0.0f;
	}
}

void ShooterEnemy::Update() {

	// --- 弾の更新は死んでいても常に行う ---
	for (auto& p : projectiles_) {
		if (p && p->IsAlive())
			p->Update();
	}
	projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(), [](const std::unique_ptr<Projectile>& p) { return !p || !p->IsAlive(); }), projectiles_.end());

	// --- 完全に死亡(kDead)なら、これ以降の移動処理は行わない ---
	if (state_ == State::kDead) {
		return;
	}

	// --- 死亡演出中(kDying)のアニメーション処理 ---
	if (state_ == State::kDying) {
		float totalDuration = kDeathSpinDuration + kDeathShrinkDuration;
		deathTimer_ += GameTime::GetDeltaTime();

		if (deathTimer_ < kDeathSpinDuration) {
			// 回転フェーズ：Y軸回転のみ
			worldTransform_.rotation_.y += kDeathSpinSpeed * GameTime::GetDeltaTime();
		} else {
			// 縮小フェーズ（回転は停止）
			float shrinkElapsed = deathTimer_ - kDeathSpinDuration;
			float t = std::clamp(shrinkElapsed / kDeathShrinkDuration, 0.0f, 1.0f);

			// スケール線形補間 1.0 -> 0.0
			float scale = (1.0f - t) * kInitialScale;
			worldTransform_.scale_ = {scale, scale, scale};

		}

		// 行列更新して終了
		TransformUpdater::WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();

		// 演出終了判定
		if (deathTimer_ >= totalDuration) {
			state_ = State::kDead;
		}
		// 死亡演出中は移動や発射を行わないのでリターン
		return;
	}

	// 左右速度は向きに従う
	velocity_.x = (lrDirection_ == LRDirection::kLeft) ? -kMoveSpeed : kMoveSpeed;

	// 移動とマップ衝突（左右のみ簡易）
	Vector3 move = {velocity_.x, 0.0f, 0.0f};
	MapCollisionRight(move);
	MapCollisionLeft(move);
	worldTransform_.translation_.x += move.x;

	if (turnTimer_ < 1.0f) {
		turnTimer_ += 1.0f / (60.0f * kTimeTurn);
		if (turnTimer_ > 1.0f) {
			turnTimer_ = 1.0f;
		}

		float distinationRotationYTable[] = {
			std::numbers::pi_v<float> * 0.5f,
			std::numbers::pi_v<float> * 1.5f
		};
		float distinationRotationY = distinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		// 線形補間 (Lerp)
		// Enemy.cppで使われている Lerp 関数があればそれを使います
		// なければ以下の計算式: start + (end - start) * t
		worldTransform_.rotation_.y = Lerp(turnFirstRotationY_, distinationRotationY, turnTimer_);
	}

	// 発射タイマー
	shootTimer_ += GameTime::GetDeltaTime();

	// --- スケール制御（反動・通常・予兆） ---

	// 発射までの残り時間
	float timeToShoot = kShootInterval - shootTimer_;

	// 1. 【反動フェーズ】 発射直後（タイマーがまだ小さい時）
	if (shootTimer_ < kRecoilDuration) {
		// 進行度 t (0.0 → 1.0)
		float t = shootTimer_ / kRecoilDuration;

		// 最大サイズ(kMaxChargeScale) から 通常サイズ(kInitialScale) へ戻る
		// 計算式: Start + (End - Start) * t
		float scale = kMaxChargeScale + (kInitialScale - kMaxChargeScale) * t;
		worldTransform_.scale_ = {scale, scale, scale};
	}
	// 2. 【攻撃予兆フェーズ】 発射直前（残り時間が1秒を切った時）
	else if (timeToShoot <= kChargeDuration && timeToShoot > 0.0f) {
		// 進行度 t (0.0 → 1.0)
		float t = 1.0f - (timeToShoot / kChargeDuration);

		// 通常サイズ(kInitialScale) から 最大サイズ(kMaxChargeScale) へ膨らむ
		float scale = kInitialScale + (kMaxChargeScale - kInitialScale) * t;
		worldTransform_.scale_ = {scale, scale, scale};
	}
	// 3. 【通常待機フェーズ】 それ以外
	else {
		worldTransform_.scale_ = {kInitialScale, kInitialScale, kInitialScale};
	}

	if (shootTimer_ >= kShootInterval && model_) {
		shootTimer_ = 0.0f;

		// 発射位置（敵のワールド座標）
		Vector3 enemyPos = {worldTransform_.matWorld_.m[3][0], worldTransform_.matWorld_.m[3][1], worldTransform_.matWorld_.m[3][2]};

		// 敵の向いている方向(lrDirection_)に応じて発射方向を決める
		Vector3 dir = {0.0f, 0.0f, 0.0f};
		if (lrDirection_ == LRDirection::kLeft) {
			dir = {-1.0f, 0.0f, 0.0f}; // 左向き
		} else {
			dir = {1.0f, 0.0f, 0.0f}; // 右向き
		}

		// 速度ベクトルを計算
		Vector3 vel = {dir.x * kProjectileSpeed, dir.y * kProjectileSpeed, dir.z * kProjectileSpeed};

		auto p = std::make_unique<Projectile>();

		// 弾の生成
		// (これまでの修正：モデル、テクスチャ、マップチップフィールドを渡す引数になっています)
		p->Initialize(projectileModel_, projectileTextureHandle_, camera_, enemyPos, vel, mapChipField_, 5.0f);

		projectiles_.push_back(std::move(p));
	}

	// 弾の更新と掃除
	for (auto& p : projectiles_) {
		if (p && p->IsAlive())
			p->Update();
	}
	projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(), [](const std::unique_ptr<Projectile>& p) { return !p || !p->IsAlive(); }), projectiles_.end());

	// 行列更新
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void ShooterEnemy::Draw() {
	if (model_ && camera_) {
		// kDead の場合は本体を描画しない（弾だけ描画するかもしれないのでreturnしない）
		if (state_ != State::kDead) {
			DirectXCommon* dxCommon = DirectXCommon::GetInstance();
			Model::PreDraw(dxCommon->GetCommandList());

			// 死亡演出中(kDying)なら色情報付きで描画
			if (state_ == State::kDying) {
				model_->Draw(worldTransform_, *camera_, textureHandle_, &objectColor_);
			} else {
				// 通常描画
				model_->Draw(worldTransform_, *camera_, textureHandle_);
			}
			Model::PostDraw();
		}
	}

	// 弾の描画（本体が死んでいても描画する）
	for (auto& p : projectiles_) {
		if (p && p->IsAlive())
			p->Draw();
	}
}

bool ShooterEnemy::GetHasAlive() const {
	// 敵本体は常に存在する想定。弾が残っているかだけ返す例。
	for (auto& p : projectiles_)
		if (p && p->IsAlive())
			return true;
	return true;
}

AABB ShooterEnemy::GetAABB() {
	Vector3 worldPos = {worldTransform_.matWorld_.m[3][0], worldTransform_.matWorld_.m[3][1], worldTransform_.matWorld_.m[3][2]};
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void ShooterEnemy::OnCollision(const Player* player) {
	(void)player;
	// ここに敵がプレイヤーに当たった時の処理（エフェクトなど）があれば書く
}

void ShooterEnemy::SetIsAlive(bool isAlive) {
	// --- Enemy.cpp と同様の状態遷移 ---
	if (isAlive) {
		// 復活・初期化
		state_ = State::kAlive;
		deathTimer_ = 0.0f;
		worldTransform_.scale_ = {kInitialScale, kInitialScale, kInitialScale};
		color_ = {1.0f, 1.0f, 1.0f, 1.0f};
		// velocity_ や onGround_ のリセットが必要ならここで行う
	} else {
		// 生存状態から死亡アニメーションへ移行
		if (state_ == State::kAlive) {
			state_ = State::kDying;
			deathTimer_ = 0.0f;
			// 動きを止める
			velocity_ = {0.0f, 0.0f, 0.0f};
			projectiles_.clear();
		}
	}
}
