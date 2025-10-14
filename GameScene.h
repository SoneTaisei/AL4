#pragma once
#include "KamataEngine.h"
#include <list>
#include "Fade.h"

class Player;
class Enemy;
class Skydome;
class MapChipField;
class CameraController;
class DeathParticles;
class Goal;

/* ゲームシーン*/
class GameScene {
private:
	/*********************************************************
	*メンバ変数
	*********************************************************/
	
	// 現在の重力方向を管理するenum
	enum class GravityDirection {
		kDown,
		kRight,
		kUp,
		kLeft,
	};
	GravityDirection gravityDirection_ = GravityDirection::kDown;

	// カメラの目標角度 Z軸
	float cameraTargetAngleZ_ = 0.0f;

	// フェーズにFadeInとFadeOutを追加
	enum class Phase {
		kFadeIn,  // フェードイン
		kPlay,    // ゲームプレイ
		kDeath,   // デス演出
		kDeathFadeOut,
		kFadeOut, // フェードアウト
	};

	// テクスチャハンドル
	uint32_t uvCheckerTextureHandle_ = 0;
	uint32_t playerTextureHandle_ = 0;
	uint32_t enemyTextureHandle_ = 0;
	uint32_t skysphereTextureHandle = 0;
	uint32_t particleTextureHandle = 0;

	// ブロックの3Dモデル
	KamataEngine::Model* cubeModel_ = nullptr;
	// 天球の3Dモデル
	KamataEngine::Model* modelSkydome_ = nullptr;
	// プレイヤーの3Dモデル
	KamataEngine::Model* playerModel_ = nullptr;
	// 敵の3Dモデル
	KamataEngine::Model* enemyModel_ = nullptr;
	// パーティクルのモデル
	KamataEngine::Model* particleModel_ = nullptr;


	// 座標変換
	KamataEngine::WorldTransform worldTransform_;
	// カメラ
	KamataEngine::Camera camera_;

	// ブロック用の座標変換。複数並べるために配列にする。std::vectorを重ねることで二次元配列にしている
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	// 自キャラ
	Player* player_ = nullptr;

	// 敵キャラ
	std::list<Enemy*> enemies_;

	// 天球
	Skydome* skydome_ = nullptr;

	// マップチップフィールド
	MapChipField* mapChipField_;

	// カメラコントローラー
	CameraController* cameraController_ = nullptr;

	// デスパーティクル
	DeathParticles* deathParticles_ = nullptr;

	// ゴールの3Dモデル
	KamataEngine::Model* goalModel_ = nullptr;
	// ゴールオブジェクト
	Goal* goal_ = nullptr;

	bool isGoal_ = false;

	// フェード
	Fade* fade_ = nullptr;

	// ゲームの現在フェーズ（変数）
	Phase phase_ = {};

	// 終了フラグ
	bool finished_ = false;

	/// <summary>
	/// 全ての当たり判定を行う
	/// </summary>
	void CheckAllCollisions();

	/// <summary>
	/// フェーズの切り替え処理
	/// </summary>
	void ChangePhase();

	/// <summary>
	/// ゲームの状態をリセットする
	/// </summary>
	void Reset();

public:
	/*********************************************************
	*メンバ関数
	*********************************************************/

	//初期化
	void Initialize(int stageNo = 1); 

	// 更新
	void Update();

	// 描画
	void Draw();

	/// <summary>
	/// マップチップデータに合わせてブロックを生成する
	/// </summary>
	void GenerateBlocks();

	// デストラクタ
	~GameScene();

	// 終了したかを取得する
	bool IsFinished() const { return finished_; }

};
