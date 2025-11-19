#pragma once
#include "KamataEngine.h"
#include <numbers>
#include "System/Collision.h"
#include "System/MapChipField.h" // 追加

// 循環参照を避けるための前方宣言
class Player;
class MapChipField;

/// <summary>
/// 敵
/// </summary>
class Enemy {
private:
	// 左右
	enum class LRDirection {
		kRight,
		kLeft,
	};

	
// マップとの当たり判定情報
	struct CollisionMapInfo {
		bool isCeilingHit = false;
		bool isLanding = false;
		bool isWallContact = false;
		KamataEngine::Vector3 move;
	};

	LRDirection lrDirection_ = LRDirection::kLeft;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;
	// 旋回時間<秒>
	static inline const float kTimeTurn = 0.2f; 

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	KamataEngine::Camera* camera_ = nullptr;

	// 歩行の速さ
	static inline const float kWorkSpeed = 0.05f;

	// 速度
	KamataEngine::Vector3 velocity_ = {};

	// 着地フラグ
	bool onGround_ = false;

	// 重力加速度
	static inline const float kGravityAcceleration = 0.02f;
	// 最大落下速度
	static inline const float kLimitFallSpeed = 0.6f;

	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// 追加: 生成時のマップインデックスを保存
	MapChipField::IndexSet spawnIndex_{UINT32_MAX, UINT32_MAX};

	// 角
	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	// 生存状態は State で管理（Alive / Dying / Dead）
	enum class State {
		kAlive,
		kDying,
		kDead
	};
	State state_ = State::kAlive;

	// --- 死亡演出用 ---
	// 死亡アニメーションの経過タイマー
	float deathTimer_ = 0.0f;
	// 回転時間（秒）
	static inline const float kDeathSpinDuration = 0.6f;
	// 縮小（フェード）時間（秒）
	static inline const float kDeathShrinkDuration = 0.4f;
	// 回転速度（ラジアン/秒）
	static inline const float kDeathSpinSpeed = 20.0f;
	// 縮小開始時スケール（通常1.0）
	static inline const float kInitialScale = 1.0f;
	// 色変更オブジェクト（フェードに使用）
	KamataEngine::ObjectColor objectColor_;
	// 色データ RGBA
	KamataEngine::Vector4 color_;

	/// <summary>
	/// 指定した角の座標を計算
	/// </summary>
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3 center, Corner corner);

	// マップ衝突判定の個別方向判定関数
	void MapCollisionUp(CollisionMapInfo& info);
	void MapCollisionDown(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);

	// アニメーション関連
	// 最初の角度[ラジアン] (-15度)
	static inline const float kWalkMotionAngleStart = -15.0f * (std::numbers::pi_v<float> / 180.0f);
	// 最後の角度[ラジアン] (+15度)
	static inline const float kWalkMotionAngleEnd = 15.0f * (std::numbers::pi_v<float> / 180.0f);
	// アニメーションの周期となる時間[秒]
	static inline const float kWalkMotionTime = 1.0f;
	// 経過時間
	float workTimer_ = 0.0f;

	// キャラクターの当たり判定サイズ (Playerに合わせて設定)
	static inline const float kWidth = 1.9f;
	static inline const float kHeight = 1.9f;

	/// <summary>
	/// ワールド座標を取得
	/// </summary>
	/// <returns>ワールド座標</returns>
	KamataEngine::Vector3 GetWorldPosition();

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデル</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	/// <param name="camera">カメラ</param>
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// マップチップフィールドをセットする
	/// </summary>
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	
	// 追加: 生成時のマップインデックスをセット/取得
	void SetSpawnIndex(const MapChipField::IndexSet& idx) { spawnIndex_ = idx; }
	MapChipField::IndexSet GetSpawnIndex() const { return spawnIndex_; }

	/// <summary>
	/// AABBを取得
	/// </summary>
	/// <returns>AABB</returns>
	AABB GetAABB();

	/// <summary>
	/// 衝突応答
	/// </summary>
	/// <param name="player">衝突相手のプレイヤー</param>
	void OnCollision(const Player* player);

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 生存フラグ（外部に対しては「通常の生存状態のみ true」を返す）
	bool GetIsAlive() const { return state_ == State::kAlive; }

	// 生存状態をセット（false を渡すと死亡アニメーションを開始）
	void SetIsAlive(bool isAlive);
};
