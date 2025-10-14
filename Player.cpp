#include "Player.h"
#include "Easing.h"
#include "Enemy.h"
#include "MapChipField.h"
#include "TransformUpdater.h"
#include <algorithm>
#include <array>
#include <numbers>

using namespace KamataEngine;

// Move関数の実装をここに追加
void Player::Move(const KamataEngine::Vector3& gravityVector) {

	// 現在の重力方向から、プレイヤーにとっての「右」と「上」を計算する (これは新しい方式のまま)
	Vector3 moveRight = {0, 0, 0};
	Vector3 moveUp = {0, 0, 0};

	if (gravityVector.y != 0) { // 重力がY軸方向
		moveRight = {1.0f, 0.0f, 0.0f};
		moveUp = {0.0f, 1.0f, 0.0f};
		if (gravityVector.y > 0) {
			moveRight.x *= -1.0f;
			moveUp.y *= -1.0f; 
		}
	} else if (gravityVector.x != 0) { // 重力がX軸方向
		moveRight = {0.0f, -1.0f, 0.0f};
		moveUp = {1.0f, 0.0f, 0.0f};
		if (gravityVector.x > 0) {
			moveRight.y *= -1.0f;
			moveUp.x *= -1.0f;
		}
	}

	// --- 左右移動 ---
	Vector3 acceleration = {};
	if (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_A)) {

		if (Input::GetInstance()->PushKey(DIK_D)) {
			acceleration = moveRight * kAcceleration;
			// ★復活★ 逆入力でのブレーキ
			float dot = velocity_.x * moveRight.x + velocity_.y * moveRight.y;
			if (dot < 0.0f) {
				velocity_ = velocity_ * (1.0f - kAttenuation);
			}

			// ★復活★ 向きを変える処理
			if (lrDirection_ != LRDirection::kRight) {
				lrDirection_ = LRDirection::kRight;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = kTimeTurn; // Player.hでkTimeTurnを0.0fから0.2fなどにすると向き転換がスムーズになります
			}
		} else if (Input::GetInstance()->PushKey(DIK_A)) {
			acceleration = moveRight * -kAcceleration;
			// ★復活★ 逆入力でのブレーキ
			float dot = velocity_.x * moveRight.x + velocity_.y * moveRight.y;
			if (dot > 0.0f) {
				velocity_ = velocity_ * (1.0f - kAttenuation);
			}

			// ★復活★ 向きを変える処理
			if (lrDirection_ != LRDirection::kLeft) {
				lrDirection_ = LRDirection::kLeft;
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = kTimeTurn;
			}
		}
		velocity_ += acceleration;
	} else {
		// --- ★改良★ 摩擦による減速 ---
		// プレイヤーの進行方向の速度だけを減速させる
		float dot = velocity_.x * moveRight.x + velocity_.y * moveRight.y;
		Vector3 runVelocity = moveRight * dot;
		Vector3 otherVelocity = velocity_ - runVelocity;
		runVelocity = runVelocity * (1.0f - kAttenuation);
		velocity_ = runVelocity + otherVelocity;
	}

	// --- ★改良★ 走行速度の制限 ---
	float runSpeedDot = velocity_.x * moveRight.x + velocity_.y * moveRight.y;
	Vector3 runVelocity = moveRight * runSpeedDot;
	Vector3 otherVelocity = velocity_ - runVelocity;
	float speed = sqrt(runVelocity.x * runVelocity.x + runVelocity.y * runVelocity.y);
	if (speed > kLimitRunSpeed) {
		runVelocity = runVelocity / speed * kLimitRunSpeed;
	}
	velocity_ = runVelocity + otherVelocity;

	// --- ジャンプと重力 (新しい方式のまま) ---
	if (onGround_) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			velocity_ += moveUp * kJumpAcceleration;
			onGround_ = false;
		}
	}

	// onGround_の如何に関わらず、常に重力を加算する
	velocity_ += gravityVector;

	// --- ★復活★ 最大落下速度の制限 ---
	// 落下方向の速度を計算 (重力と逆方向が上なので、上方向との内積の逆が落下速度)
	float fallSpeed = -(velocity_.x * moveUp.x + velocity_.y * moveUp.y);
	if (fallSpeed > kLimitFallSpeed) {
		// 落下方向の速度成分だけを制限値にクランプする
		Vector3 fallVelocity = -moveUp * fallSpeed;
		Vector3 sideVelocity = velocity_ - fallVelocity;
		fallVelocity = -moveUp * kLimitFallSpeed;
		velocity_ = sideVelocity + fallVelocity;
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
		//   ↓ 以下の行を追加
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
		//   ↓ 以下の行を追加
		float targetPlayerCenterX = blockRect.right + kWidth / 2.0f;
		info.move.x = targetPlayerCenterX - worldTransform_.translation_.x;
	}
}

// MapCollision関数の実装 (変更なし)
void Player::MapCollision(CollisionMapInfo& info) {
	// 各方向の衝突判定関数を呼び出す
	MapCollisionUp(info);
	MapCollisionDown(info);
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		MapCollisionRight(info);
	}
	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
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
KamataEngine::Vector3 Player::GetWorldPosition() {
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

void Player::OnCollision(const Enemy* enemy) {
	// 衝突相手の情報を表示 (デバッグ用)
	(void)enemy; // 今は使わないので警告抑制

	// 敵に当たったら死亡フラグを立てる
	isDead_ = true;
}

void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
#ifdef _DEBUG
	// NULLポインタチェック
	assert(model);
#endif // _DEBUG

	// 因数として受け取ったデータをメンバ変数に記録
	playerModel_ = model;
	playerTextureHandle_ = textureHandle;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	// 座標を元に行列の更新を行う
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	// 速度をゼロクリアする
	velocity_ = {0.0f, 0.0f, 0.0f};
	// 接地フラグをリセット
	onGround_ = false;

	// 生存状態で初期化
	isDead_ = false;
}

void Player::Update(const KamataEngine::Vector3& gravityVector, float cameraAngleZ) {

	if (isDead_) {
		return;
	}

	// 毎フレームのプレイヤーの基本情報を出力
	std::string debugText = std::format(
	    "Player Pos:({:.2f}, {:.2f}), Vel:({:.2f}, {:.2f}), onGround:{}, Grav:({:.2f}, {:.2f})\n", worldTransform_.translation_.x, worldTransform_.translation_.y, velocity_.x, velocity_.y, onGround_,
	    gravityVector.x, gravityVector.y);
	OutputDebugStringA(debugText.c_str());

	// 1. 移動と重力の計算（当たり判定前の速度がここで決まる）
	Move(gravityVector);

	// 2. 当たり判定と移動量の補正
	// 重力方向に応じて、衝突判定の軸を入れ替える
	if (gravityVector.y != 0) { // 重力が上下方向の場合 (従来通り)
		// X軸（水平）の衝突判定と移動
		{
			CollisionMapInfo infoX{};
			infoX.move = {velocity_.x, 0.0f, 0.0f};
			MapCollisionRight(infoX);
			MapCollisionLeft(infoX);
			if (infoX.isWallContact) {
				velocity_.x = 0.0f;
			}
			worldTransform_.translation_.x += infoX.move.x;
		}

		// Y軸（垂直）の衝突判定と移動
		{
			CollisionMapInfo infoY{};
			infoY.move = {0.0f, velocity_.y, 0.0f};
			MapCollisionUp(infoY);
			MapCollisionDown(infoY);
			worldTransform_.translation_.y += infoY.move.y;

			// 1. 通常の床への着地 (重力が下向きのとき)
			bool landedOnFloor = (gravityVector.y < 0 && infoY.isLanding);
			// 2. 天井への"着地" (重力が上向きのとき)
			bool landedOnCeiling = (gravityVector.y > 0 && infoY.isCeilingHit);

			// 床か天井、どちらかに着地していれば onGround を true にする
			if (landedOnFloor || landedOnCeiling) {
				onGround_ = true;
				velocity_.y = 0.0f;
			} else {
				onGround_ = false;
			}

			// 3. ジャンプして頭をぶつけた時の処理
			// (通常重力で天井にヒット OR 上向き重力で床にヒット)
			if ((gravityVector.y < 0 && infoY.isCeilingHit) || (gravityVector.y > 0 && infoY.isLanding)) {
				velocity_.y = 0.0f;
			}
		}
	} else if (gravityVector.x != 0) { // 重力が左右方向の場合
		// Y軸（プレイヤーにとっての水平）の衝突判定と移動
		{
			// Y方向の速度がごくわずかな場合は、水平判定をスキップする
			if (std::abs(velocity_.y) > 0.001f) {
				CollisionMapInfo infoY{};
				infoY.move = {0.0f, velocity_.y, 0.0f};
				// MapCollisionUp/Down が壁判定の役割を果たす
				MapCollisionUp(infoY);
				MapCollisionDown(infoY);
				// 天井か床に当たったら、それは壁接触とみなす
				if (infoY.isCeilingHit || infoY.isLanding) {
					velocity_.y = 0.0f;
				}
				worldTransform_.translation_.y += infoY.move.y;
			}
		}

		// X軸（プレイヤーにとっての垂直）の衝突判定と移動
		{
			CollisionMapInfo infoX{};
			infoX.move = {velocity_.x, 0.0f, 0.0f};
			// MapCollisionRight/Left が接地/頭上判定の役割を果たす
			MapCollisionRight(infoX);
			MapCollisionLeft(infoX);
			worldTransform_.translation_.x += infoX.move.x;

			// 接地判定 (重力と同じ方向の壁に、同じ方向に移動して接触した場合)
			bool isLandingX = (gravityVector.x > 0 && infoX.isWallContact && velocity_.x >= 0) || // 右向き重力で右壁に接触
			                  (gravityVector.x < 0 && infoX.isWallContact && velocity_.x <= 0);   // 左向き重力で左壁に接触

			// 天井判定 (重力と逆方向の壁に、逆方向に移動して接触した場合)
			bool isCeilingHitX = (gravityVector.x > 0 && infoX.isWallContact && velocity_.x < 0) || // 右向き重力で左壁に接触
			                     (gravityVector.x < 0 && infoX.isWallContact && velocity_.x > 0);   // 左向き重力で右壁に接触

			// X軸重力時の判定結果を詳しく出力
			if (infoX.isWallContact) { // 壁に接触したときだけログを出す
				std::string xGravDebugText =
				    std::format("  [X-Grav Check] WallContact:{}, Vel.x:{:.2f} -> isLandingX:{}, isCeilingHitX:{}\n", infoX.isWallContact, velocity_.x, isLandingX, isCeilingHitX);
				OutputDebugStringA(xGravDebugText.c_str());
			}

			if (isLandingX) {
				onGround_ = true;
				velocity_.x = 0.0f;
			} else {
				// isLandingXがfalseでも、天井に当たっていないならonGround_をfalseにする
				// (空中でもう一方の壁に当たった場合に接地判定が残るのを防ぐ)
				if (!isCeilingHitX) {
					onGround_ = false;
				}
			}
			if (isCeilingHitX) {
				velocity_.x = 0.0f;
			}
		}
	}

	// 3. 向きの更新と行列の転送
	if (turnTimer_ < 1.0f) {
		turnTimer_ += 1.0f / 20.0f;
		float destRotYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float destRotY = destRotYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotation_.y = Lerp(turnFirstRotationY_, destRotY, turnTimer_);
	}

	// カメラのZ軸回転をプレイヤーのZ軸回転に反映させる
	worldTransform_.rotation_.z = cameraAngleZ;

	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();

	// 座標を元に行列の更新を行う
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Player::Draw() {
	// 死亡していたら以降の処理は行わない
	if (isDead_) {
		return;
	}

	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	// 3Dモデルを描画
	playerModel_->Draw(worldTransform_, *camera_, playerTextureHandle_);

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();
}
