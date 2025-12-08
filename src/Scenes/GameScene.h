#pragma once
#include "Effects/Fade.h"
#include "KamataEngine.h"
#include <list>

class Player;
class Enemy;
class ChasingEnemy;
class ShooterEnemy;
class Skydome;
class MapChipField;
class CameraController;
class DeathParticles;
class Goal;
class UI;
class HUD;

/* ゲームシーン*/
class GameScene {
private:
	/*********************************************************
	 *メンバ変数
	 *********************************************************/

	enum class GravityDirection {
		kDown,
		kRight,
		kUp,
		kLeft,
	};
	GravityDirection gravityDirection_ = GravityDirection::kDown;

	// カメラの目標角度 Z軸
	float cameraTargetAngleZ_ = 0.0f;

	// kStageStart
	enum class Phase {
		kStageStart, // ステージ開始（暗転・番号表示）
		kFadeIn,     // フェードイン
		kPlay,       // ゲームプレイ
		kDeath,      // デス演出
		kDeathFadeOut,
		kFadeOut, // フェードアウト
	};

	// テクスチャハンドル
	uint32_t uvCheckerTextureHandle_ = 0;
	uint32_t playerTextureHandle_ = 0;
	uint32_t enemyTextureHandle_ = 0;
	uint32_t chasingEnemyTextureHandle_ = 0;
	uint32_t shooterEnemyTextureHandle_ = 0;
	uint32_t skysphereTextureHandle = 0;
	uint32_t particleTextureHandle = 0;
	uint32_t projectileTextureHandle_ = 0;

	// モデル
	KamataEngine::Model* cubeModel_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::Model* enemyModel_ = nullptr;
	KamataEngine::Model* chasingEnemyModel_ = nullptr;
	KamataEngine::Model* shooterEnemyModel_ = nullptr;
	KamataEngine::Model* particleModel_ = nullptr;
	KamataEngine::Model* projectileModel_ = nullptr;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Camera camera_;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	Player* player_ = nullptr;
	std::list<Enemy*> enemies_;
	std::list<ChasingEnemy*> chasingEnemies_;
	std::list<ShooterEnemy*> shooterEnemies_;
	Skydome* skydome_ = nullptr;
	MapChipField* mapChipField_;
	CameraController* cameraController_ = nullptr;
	DeathParticles* deathParticles_ = nullptr;
	KamataEngine::Model* goalModel_ = nullptr;
	Goal* goal_ = nullptr;
	bool isGoal_ = false;

	Fade* fade_ = nullptr;
	Phase phase_ = {};
	bool finished_ = false;

	HUD* HUD_ = nullptr;
	UI* UI_ = nullptr;

	uint32_t jHandle_;
	KamataEngine::Sprite* jSprite_ = {};
	uint32_t spaceHandle_;
	KamataEngine::Sprite* spaceSprite_ = {};

	bool isPaused_ = false;
	bool showControls_ = false;
	int pauseMenuIndex_ = 0;
	static inline const int kPauseMenuCount = 2;

	// 現在のステージ番号
	int currentStageNo_ = 1;
	// ステージ開始時のタイマー
	float stageStartTimer_ = 0.0f;

	void CheckAllCollisions();
	void ChangePhase();
	void Reset();

public:
	void Initialize(int stageNo = 1);
	void Update();
	void Draw();
	void GenerateBlocks();
	~GameScene();

	bool GetIsFinished() const { return finished_; }
	void SetIsFinished(bool finished) { finished_ = finished; }
};