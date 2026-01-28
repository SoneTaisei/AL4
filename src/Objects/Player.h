#pragma once
#include "KamataEngine.h"
#include "System/Collision.h"
#include "Utils/Easing.h"

class TransformUpdater;
class MapChipField;
class Enemy;
class ChasingEnemy;
class ShooterEnemy;

// マップとの当たり判定情報
struct CollisionMapInfo {
	bool isCeilingHit = false;  // 天井衝突フラグ
	bool isLanding = false;     // 着地フラグ
	bool isWallContact = false; // 壁接触フラグ
	KamataEngine::Vector3 move; // 移動量
};

// 演出の状態定義
enum class GoalAnimationPhase {
	kNone,
	kSpin, // 一回転して正面を向く
	kWait, // 回転後の待機時間
	kJump, // ジャンプ
	kPose, // ポーズをとるフェーズ
	kEnd   // 演出終了
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
	// ダッシュ速度
	static inline const float kDashSpeed = 0.8f;

	// ダッシュ攻撃の速度
	static inline const float kDashAttackSpeed = 1.2f;

	// ダッシュ中か
	bool isDashing_ = false;

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

	//ジャンプ回数
	const int kMaxJumpCount = 2;
	int jumpCount = 0;

	// 攻撃中フラグ
	bool isAttacking_ = false;

	// 攻撃タイマー
	float attackTimer_ = 0.0f;
	// ★★★ 攻撃の時間を分割して定義 ★★★
	// 攻撃の「タメ」の時間 <秒>
	static inline const float kAttackSquashDuration = 0.1f;
	// 攻撃の「伸び」の時間 <秒>
	static inline const float kAttackStretchDuration = 0.3f;
	// 攻撃の合計時間
	static inline const float kAttackDuration = kAttackSquashDuration + kAttackStretchDuration;

	// ★★★ モーションの強さを定義 ★★★
	// タメモーションのY方向の縮み量 (例: 0.5f -> 元の50%まで縮む)
	static inline const float kSquashAmountY = 0.5f;
	// 伸びモーションのY方向の伸び量 (例: 0.5f -> 元の1.5倍まで伸びる)
	static inline const float kStretchAmountY = 0.5f;
	// 攻撃の突進距離
	static inline const float kAttackDistance = 8.0f;
	// 攻撃開始時の座標
	KamataEngine::Vector3 attackStartPosition_ = {};
	bool isAttackBlocked_ = false;

	// 近接攻撃
	bool isMeleeAttacking_ = false;
	float meleeAttackTimer_ = 0.0f;
	static inline const float kMeleeAttackDuration = 0.6f;
	static inline const float kMeleeAttackRange = 3.0f;
	static inline const float kMeleeAttackMoveDistance = 2.0f;

	// マップチップによるフィールド
	MapChipField* mapChipFiled_ = nullptr;

	const std::list<Enemy*>* enemies_ = nullptr;
	const std::list<ChasingEnemy*>* chasingEnemies_ = nullptr;
	const std::list<ShooterEnemy*>* shooterEnemies_ = nullptr;

	// 死亡フラグ
	bool isAlive_ = false;

	// 無敵
	bool isInvincible_ = false;

	// ノックバックの強さ
	static inline const float kKnockbackHorizontalPower = 0.3f; // 水平方向の強さ
	static inline const float kKnockbackVerticalPower = 0.2f;   // 少し上に跳ねる強さ

	// 現在の重力ベクトル
	KamataEngine::Vector3 gravity_ = { 0.0f, -kGravityAcceleration, 0.0f };

	// HP
	int hp_ = 0;
	static inline const int kMaxHp = 3; // 最大HP

	// 死亡演出中フラグ
	bool isDeadAnimating_ = false;

	// 死亡演出用タイマー
	float deathTimer_ = 0.0f;
	// 死亡演出の時間（秒）。この時間が経過するとGameScene側で死亡と判定される
	static inline const float kDeathAnimationDuration = 2.0f;

	// ダメージ量
	static inline const int kDamageFromEnemy = 1;

	// 無敵時間タイマー
	float invincibleTimer_ = 0.0f;
	// 無敵時間 <秒>
	static inline const float kInvincibleDuration = 2.0f;

	// ゴール演出用
	GoalAnimationPhase goalAnimationPhase_ = GoalAnimationPhase::kNone;
	float goalAnimTimer_ = 0.0f;

	float goalStartRotationY_ = 0.0f; // 演出開始時の角度を保存

	// ポーズ開始時の角度保存用
	float goalStartRotationZ_ = 0.0f;

	// 角
	enum Corner {
		kRightBottom, // 右下
		kLeftBottom,  // 左下
		kRightTop,    // 右上
		kLeftTop,     // 左上
		kNumCorner    // 要素数
	};

	// 剣に関する変数
	KamataEngine::Model* swordModel_ = nullptr;
	uint32_t swordTextureHandle_ = 0u;
	KamataEngine::WorldTransform swordWorldTransform_;
	// 攻撃時の体の傾き
	float attackTilt_ = 0.0f;

	/// <summary>
	/// 指定した角の座標を計算
	/// </summary>
	/// <param name="center">中心座標</param>
	/// <param name="corner">角の種類</param>
	/// <returns>指定した角の座標</returns>
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3 center, Corner corner);

	/// <summary>
	/// 攻撃処理
	/// </summary>
	void Attack(const KamataEngine::Vector3& gravityVector);

	/// <summary>
	/// 衝突を考慮しながら指定された量だけ移動する
	/// </summary>
	/// <param name="move">移動量</param>
	/// <param name="gravityVector">現在の重力ベクトル</param>
	/// <returns>衝突が発生した場合 true</returns>
	bool MoveAndCollide(const KamataEngine::Vector3& move, const KamataEngine::Vector3& gravityVector);

	/// <summary>
	/// マップ衝突判定
	/// </summary>
	/// <param name="info">衝突情報</param>
	void MapCollision(CollisionMapInfo& info);

	// マップ衝突判定の個別方向判定関数
	void MapCollisionUp(CollisionMapInfo& info); // 上方向判定
	void MapCollisionDown(CollisionMapInfo& info); // 下方向判定 
	void MapCollisionRight(CollisionMapInfo& info); // 右方向判定
	void MapCollisionLeft(CollisionMapInfo& info); // 左方向判定 

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

	/// <summary>
	/// 攻撃に関する状態更新（モーション、移動量計算、入力受付など）
	/// </summary>
	/// <param name="gravityVector">現在の重力</param>
	/// <param name="outAttackMove">計算された攻撃の移動量（出力用）</param>
	void UpdateAttack(const KamataEngine::Vector3& gravityVector, KamataEngine::Vector3& outAttackMove);

	void MeleeAttack();

	/// <summary>
	/// キー入力に応じて速度を更新する
	/// </summary>
	/// <param name="gravityVector">現在の重力</param>
	void UpdateVelocityByInput(const KamataEngine::Vector3& gravityVector);

	/// <summary>
	/// 最終的な移動量を元に、衝突判定を行いながら座標を更新する
	/// </summary>
	/// <param name="finalMove">最終的な移動量</param>
	/// <param name="gravityVector">現在の重力</param>
	void ApplyCollisionAndMove(const KamataEngine::Vector3& finalMove, const KamataEngine::Vector3& gravityVector);

	/// <summary>
	/// 向きの更新とワールド行列の計算
	/// </summary>
	/// <param name="cameraAngleZ">カメラのZ軸回転</param>
	void UpdateRotationAndTransform(float cameraAngleZ);

	/// <summary>
	/// 移動入力 (古い名前、UpdateVelocityByInputに役割を移譲)
	/// </summary>
	/// <returns></returns>
	// void Move(const KamataEngine::Vector3& gravityVector); // ← 分かりやすい名前に変更したのでコメントアウト or 削除

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデル</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	/// <param name="camera">カメラ</param>
	void Initialize(
	    KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Model* swordModel, uint32_t swordTextureHandle, 
	    KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(
	    const KamataEngine::Vector3& gravityVector, float cameraAngleZ, const std::list<Enemy*>& enemies,
	    const std::list<ChasingEnemy*>& chasingEnemies, const std::list<ShooterEnemy*>& shooterEnemies, float timeScale = 1.0f);

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
	KamataEngine::Vector3 GetWorldPosition() const;

	/// <summary>
	/// AABBを取得
	/// </summary>
	/// <returns>AABB</returns>
	AABB GetAABB();

	/// <summary>
	/// 衝突応答
	/// </summary>
	/// <param name="enemy">衝突相手の敵</param>
	void OnCollision(const KamataEngine::WorldTransform &worldTransform);

	/// <summary>
	/// ゴール演出開始
	/// </summary>
	void StartGoalAnimation();

	/// <summary>
	/// ゴール演出更新
	/// </summary>
	void UpdateGoalAnimation();

	/// <summary>
	/// ゴール演出の状態を取得
	/// </summary>
	GoalAnimationPhase GetGoalAnimationPhase() const { return goalAnimationPhase_; }

	/// <summary>
	/// HPを取得する
	/// </summary>
	int GetHp() const { return hp_; }

	// 死亡状態を取得する
	bool GetIsAlive() const { return isAlive_; }

	// 無敵状態を取得する
	bool GetIsInvincible() const { return isInvincible_; }

	bool GetIsAttacking() const { return isAttacking_; }

	static float GetGravityAcceleration() { return kGravityAcceleration; }

	// 死亡演出中かどうかを取得する（追加）
	bool GetIsDeadAnimating() const { return isDeadAnimating_; }



};
