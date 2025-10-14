#pragma once
#include "KamataEngine.h"
#include "Collision.h"

class TransformUpdater;
class MapChipField;
class Enemy;

// マップとの当たり判定情報
struct CollisionMapInfo {
	bool isCeilingHit = false;  // 天井衝突フラグ
	bool isLanding = false;     // 着地フラグ
	bool isWallContact = false; // 壁接触フラグ
	KamataEngine::Vector3 move; // 移動量
};

/// <summary>
/// 自キャラ
/// </summary>
class Player {
private:
	// 左右
	enum class LRDirection {
		kRight,
		kLeft,
	};

	LRDirection lrDirection_ = LRDirection::kRight;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* playerModel_ = nullptr;
	// テクスチャハンドル
	uint32_t playerTextureHandle_ = 0u;

	KamataEngine::Camera* camera_ = nullptr;

	// 速度
	KamataEngine::Vector3 velocity_ = {0.0f,-1.0f,0.0f};
	// 加速度
	static inline const float kAcceleration = 0.01f;
	// 減速率
	static inline const float kAttenuation = 0.05f;
	// 最大速度
	static inline const float kLimitRunSpeed = 0.5f;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 2.0f;
	static inline const float kHeight = 2.0f;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;
	// 旋回時間<秒>
	static inline const float kTimeTurn = 0.0f;

	// ジャンプフラグ
	bool onGround_ = false;

	// 重力加速度
	static inline const float kGravityAcceleration = 0.02f;
	// 最大落下速度
	static inline const float kLimitFallSpeed = 0.6f;
	// ジャンプ初速
	static inline const float kJumpAcceleration = 0.5f;

	// マップチップによるフィールド
	MapChipField* mapChipFiled_ = nullptr;

	// 死亡フラグ
	bool isDead_ = false;

	// 角
	enum Corner {
		kRightBottom, // 右下
		kLeftBottom,  // 左下
		kRightTop,    // 右上
		kLeftTop,     // 左上
		kNumCorner    // 要素数
	};

	/// <summary>
	/// 指定した角の座標を計算
	/// </summary>
	/// <param name="center">中心座標</param>
	/// <param name="corner">角の種類</param>
	/// <returns>指定した角の座標</returns>
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3 center, Corner corner);


	/// <summary>
	/// 移動入力
	/// </summary>
	void Move(const KamataEngine::Vector3& gravityVector); // 新しい関数の宣言

	/// <summary>
	/// マップ衝突判定
	/// </summary>
	/// <param name="info">衝突情報</param>
	void MapCollision(CollisionMapInfo& info);

	// マップ衝突判定の個別方向判定関数
	void MapCollisionUp(CollisionMapInfo& info); // 上方向判定
	void MapCollisionDown(CollisionMapInfo& info); // 下方向判定 (後で追加)
	void MapCollisionRight(CollisionMapInfo& info); // 右方向判定 (後で追加)
	void MapCollisionLeft(CollisionMapInfo& info); // 左方向判定 (後で追加)

	/// <summary>
	/// 判定結果を反映して移動させる
	/// </summary>
	/// <param name="info">衝突情報</param>
	void ReflectCollisionResultAndMove(const CollisionMapInfo& info);

	/// <summary>
	/// 天井に接触している場合の処理
	/// </summary>
	/// <param name="info">衝突情報</param>
	void HandleCeilingCollision(const CollisionMapInfo& info);

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデル</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	/// <param name="camera">カメラ</param>
	void Initialize(KamataEngine::Model*model,uint32_t textureHandle,KamataEngine::Camera*camera,const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(const KamataEngine::Vector3& gravityVector, float cameraAngleZ);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// ワールド変換データを取得
	/// </summary>
	/// <returns>ワールド変換データ</returns>
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	void SetMapChipField(MapChipField* mapChipFiled) { mapChipFiled_ = mapChipFiled; }

	/// <summary>
	/// ワールド座標を取得
	/// </summary>
	/// <returns>ワールド座標</returns>
	KamataEngine::Vector3 GetWorldPosition();

	/// <summary>
	/// AABBを取得
	/// </summary>
	/// <returns>AABB</returns>
	AABB GetAABB();

	/// <summary>
	/// 衝突応答
	/// </summary>
	/// <param name="enemy">衝突相手の敵</param>
	void OnCollision(const Enemy* enemy);

	// 死亡状態を取得する
	bool IsDead() const { return isDead_; }

	// public にゲッターを追加
	static float GetGravityAcceleration() { return kGravityAcceleration; }

};
