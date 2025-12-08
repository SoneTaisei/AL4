#include "GameScene.h"
#include "Effects/DeathParticles.h"
#include "Effects/Fade.h"
#include "Effects/Skydome.h"
#include "HUD/HUD.h"
#include "Objects/ChasingEnemy.h"
#include "Objects/Enemy.h"
#include "Objects/Goal.h"
#include "Objects/Player.h"
#include "Objects/ShooterEnemy.h"
#include "System/CameraController.h"
#include "System/GameTime.h"
#include "System/MapChipField.h"
#include "UI/UI.h"
#include "Utils/TransformUpdater.h"
#include <Windows.h>
#include <cstdio>
#include <imgui.h>

using namespace KamataEngine;

/// <summary>
/// AABB同士の交差判定
/// </summary>
bool IsColliding(const AABB& aabb1, const AABB& aabb2) {
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) && (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) && (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z)) {
		return true;
	}
	return false;
}

void GameScene::Reset() {
	// プレイヤーの初期化
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 9); // フォールバック

	MapChipField::IndexSet startIdx;
	if (mapChipField_->FindFirstIndexByType(MapChipType::kPlayerStart, startIdx)) {
		playerPosition = mapChipField_->GetMapChipPositionByIndex(startIdx.xIndex, startIdx.yIndex);
	}

	player_->Initialize(playerModel_, playerTextureHandle_, &camera_, playerPosition);
	player_->SetMapChipField(mapChipField_);

	// 敵の初期化
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	// 追尾する敵の初期化
	for (ChasingEnemy* enemy : chasingEnemies_) {
		delete enemy;
	}
	chasingEnemies_.clear();

	// 射撃する敵の初期化
	for (ShooterEnemy* enemy : shooterEnemies_) {
		delete enemy;
	}
	shooterEnemies_.clear();

	// マップから敵を生成
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical(); ++y) {
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x) {
			MapChipType t = mapChipField_->GetMapChipTypeByIndex(x, y);
			if (t == MapChipType::kEnemy) {
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(x, y);
				Enemy* newEnemy = new Enemy();
				newEnemy->Initialize(enemyModel_, enemyTextureHandle_, &camera_, enemyPosition);
				newEnemy->SetMapChipField(mapChipField_);
				newEnemy->SetSpawnIndex(mapChipField_->GetMapChipIndexSetByPosition(enemyPosition));
				enemies_.push_back(newEnemy);
			} else if (t == MapChipType::kChasingEnemy) {
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(x, y);
				ChasingEnemy* newEnemy = new ChasingEnemy();
				newEnemy->Initialize(chasingEnemyModel_, chasingEnemyTextureHandle_, &camera_, enemyPosition);
				newEnemy->SetTargetPlayer(player_);
				chasingEnemies_.push_back(newEnemy);
			} else if (t == MapChipType::kShooter) {
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(x, y);
				ShooterEnemy* newEnemy = new ShooterEnemy();
				newEnemy->Initialize(shooterEnemyModel_, projectileModel_, shooterEnemyTextureHandle_, projectileTextureHandle_, &camera_, enemyPosition);
				newEnemy->SetMapChipField(mapChipField_);
				newEnemy->SetPlayer(player_);
				shooterEnemies_.push_back(newEnemy);
			}
		}
	}

	// カメラコントローラーのリセット
	cameraController_->SetTarget(player_);
	cameraController_->Reset();
	cameraTargetAngleZ_ = 0.0f;

	// ★リセット後はまず「ステージ開始」フェーズへ
	phase_ = Phase::kFadeIn;
	stageStartTimer_ = 0.0f;

	// FadeInで初期化するが、Updateは呼ばず「不透明(真っ黒)」のままキープする
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void GameScene::Initialize(int stageNo) {
	// ★ステージ番号を保存
	currentStageNo_ = stageNo;

	// テクスチャの読み込み
	uvCheckerTextureHandle_ = TextureManager::Load("uvChecker.png");
	playerTextureHandle_ = TextureManager::Load("AL3_Player/Player.png");
	enemyTextureHandle_ = TextureManager::Load("AL3_Enemy/Enemy.png");
	chasingEnemyTextureHandle_ = TextureManager::Load("AL3_ChasingEnemy/ChasingEnemy.png");
	shooterEnemyTextureHandle_ = TextureManager::Load("AL3_ShooterEnemy/ShooterEnemy.png");
	skysphereTextureHandle = TextureManager::Load("skydome/AL_skysphere.png");
	particleTextureHandle = TextureManager::Load("AL3_Particle/AL3_Particle.png");
	projectileTextureHandle_ = TextureManager::Load("ball/ball.png");

	// 3Dモデルの生成
	cubeModel_ = Model::CreateFromOBJ("debugCube");
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	playerModel_ = Model::CreateFromOBJ("AL3_Player", true);
	enemyModel_ = Model::CreateFromOBJ("AL3_Enemy", true);
	chasingEnemyModel_ = Model::CreateFromOBJ("AL3_ChasingEnemy", true);
	shooterEnemyModel_ = Model::CreateFromOBJ("AL3_ShooterEnemy", true);
	particleModel_ = Model::CreateFromOBJ("AL3_Particle", true);
	goalModel_ = Model::CreateFromOBJ("goal", true);
	projectileModel_ = Model::CreateFromOBJ("ball", true);

	// 座標変換の初期化
	worldTransform_.Initialize();
	// カメラの初期化
	camera_.Initialize();
	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// マップチップフィールドの生成
	mapChipField_ = new MapChipField();
	std::string mapFileName = "Resources/stage/stage" + std::to_string(stageNo) + ".csv";
	mapChipField_->LoadMapChipCsv(mapFileName);

	// ゴールとプレイヤーの座標設定
	Vector3 goalPosition = mapChipField_->GetMapChipPositionByIndex(49, 3);
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 9);
	MapChipField::IndexSet startIdx;
	if (mapChipField_->FindFirstIndexByType(MapChipType::kPlayerStart, startIdx)) {
		playerPosition = mapChipField_->GetMapChipPositionByIndex(startIdx.xIndex, startIdx.yIndex);
	}
	// ステージごとの調整（互換性のため）
	switch (stageNo) {
	case 1:
		goalPosition = mapChipField_->GetMapChipPositionByIndex(49, 3);
		break;
	case 2:
	case 3:
		goalPosition = mapChipField_->GetMapChipPositionByIndex(37, 10);
		break;
	default:
		goalPosition = mapChipField_->GetMapChipPositionByIndex(5, 4);
		break;
	}

	// 自キャラの生成と初期化
	player_ = new Player();
	player_->Initialize(playerModel_, playerTextureHandle_, &camera_, playerPosition);

	// デスパーティクルの生成と初期化
	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(particleModel_, particleTextureHandle, &camera_, playerPosition);

	// 敵をマップチップから生成
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical(); ++y) {
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x) {
			MapChipType t = mapChipField_->GetMapChipTypeByIndex(x, y);
			if (t == MapChipType::kEnemy) {
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(x, y);
				Enemy* newEnemy = new Enemy();
				newEnemy->Initialize(enemyModel_, enemyTextureHandle_, &camera_, enemyPosition);
				newEnemy->SetMapChipField(mapChipField_);
				newEnemy->SetSpawnIndex(mapChipField_->GetMapChipIndexSetByPosition(enemyPosition));
				enemies_.push_back(newEnemy);
			} else if (t == MapChipType::kChasingEnemy) {
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(x, y);
				ChasingEnemy* newEnemy = new ChasingEnemy();
				newEnemy->Initialize(chasingEnemyModel_, chasingEnemyTextureHandle_, &camera_, enemyPosition);
				newEnemy->SetTargetPlayer(player_);
				chasingEnemies_.push_back(newEnemy);
			} else if (t == MapChipType::kShooter) {
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(x, y);
				ShooterEnemy* newEnemy = new ShooterEnemy();
				newEnemy->Initialize(shooterEnemyModel_, projectileModel_, shooterEnemyTextureHandle_, projectileTextureHandle_, &camera_, enemyPosition);
				newEnemy->SetMapChipField(mapChipField_);
				newEnemy->SetPlayer(player_);
				shooterEnemies_.push_back(newEnemy);
			}
		}
	}

	// ブロック生成
	GenerateBlocks();

	// 天球の生成と初期化
	skydome_ = new Skydome();
	skydome_->Initialize(modelSkydome_, skysphereTextureHandle, &camera_);

	// プレイヤーにマップチップフィールドをセット
	player_->SetMapChipField(mapChipField_);

	// ゴールの生成と初期化
	goal_ = new Goal();
	goal_->Initialize(goalModel_, goalPosition);

	// カメラコントローラーの生成と初期化
	cameraController_ = new CameraController();
	cameraController_->Initialize(&camera_);
	cameraController_->SetTarget(player_);
	cameraController_->Reset();

	Rect cameraMovableArea;
	cameraMovableArea.left = 0.0f;
	cameraMovableArea.right = 200.0f;
	cameraMovableArea.bottom = 0.0f;
	cameraMovableArea.top = 100.0f;
	cameraController_->SetMovableArea(cameraMovableArea);

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// ★ステージ開始フェーズからスタート
	phase_ = Phase::kStageStart;
	stageStartTimer_ = 0.0f;

	// FadeIn開始状態（不透明度1.0 = 真っ黒）にする
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	finished_ = false;

	UI_ = new UI;
	UI_->Initialize();

	HUD_ = new HUD;
	HUD_->Initialize();

	jHandle_ = TextureManager::GetInstance()->Load("HUD/J.png");
	jSprite_ = Sprite::Create(jHandle_, {64, 600});
	if (jSprite_) {
		jSprite_->SetSize({64, 64});
	}
	spaceHandle_ = TextureManager::GetInstance()->Load("HUD/space.png");
	spaceSprite_ = Sprite::Create(spaceHandle_, {192, 600});
	if (spaceSprite_) {
		spaceSprite_->SetSize({224, 64});
	}
}

void GameScene::Update() {

	// ポーズ切替
	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		isPaused_ = !isPaused_;
		if (!isPaused_) {
			showControls_ = false;
		}
	}

	// --- フェーズごとの処理 ---
	switch (phase_) {
	case Phase::kStageStart:
		// ★ステージ開始演出（暗転＆番号表示）
		stageStartTimer_ += GameTime::GetDeltaTime();

		// フェードのUpdateは呼ばず、真っ黒の状態を維持する

		cameraController_->Update(); // カメラは動かしておく

		// 3秒経過したらフェードイン開始
		if (stageStartTimer_ >= 3.0f) {
			phase_ = Phase::kFadeIn;
			// Startは既にInitialize/Resetで呼ばれているので、ここで再度呼ぶ必要はない
			// （counterが0のままなので、ここからUpdateを呼べばフェードインが始まる）
		}
		break;

	case Phase::kFadeIn:
		// フェード更新（徐々に明るくなる）
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kPlay;
		}
		cameraController_->Update();
		break;

	case Phase::kPlay: {
		// ポーズ中はゲームプレイ更新を停止してImGuiでメニュー表示
		if (isPaused_) {
			if (Input::GetInstance()->TriggerKey(DIK_W)) {
				pauseMenuIndex_ = (pauseMenuIndex_ + GameScene::kPauseMenuCount - 1) % GameScene::kPauseMenuCount;
			}
			if (Input::GetInstance()->TriggerKey(DIK_S)) {
				pauseMenuIndex_ = (pauseMenuIndex_ + 1) % GameScene::kPauseMenuCount;
			}
			if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_RETURN)) {
				if (pauseMenuIndex_ == 0) {
					Reset();
					isPaused_ = false;
					showControls_ = false;
					return;
				} else if (pauseMenuIndex_ == 1) {
					finished_ = true;
				}
			}

#ifdef _DEBUG
			ImGui::Begin("ポーズメニュー");
			ImGui::Text("ポーズ中");
			if (ImGui::Button("操作確認")) {
				showControls_ = !showControls_;
			}
			if (showControls_) {
				ImGui::Separator();
				ImGui::TextWrapped("操作方法:\n← → : 移動\nSPACE : ジャンプ\nR : リセット\nESC : ポーズ切替");
			}
			ImGui::Separator();
			ImGui::Text("↑/↓ で選択, SPACE/Enter で決定");
			if (ImGui::Selectable("1: リスタート", pauseMenuIndex_ == 0)) {
				pauseMenuIndex_ = 0;
				Reset();
				isPaused_ = false;
				showControls_ = false;
				ImGui::End();
				return;
			}
			if (ImGui::Selectable("2: ステージセレクトに戻る", pauseMenuIndex_ == 1)) {
				pauseMenuIndex_ = 1;
				finished_ = true;
			}
			ImGui::End();
#endif
			break;
		}

		GameTime::Update();

		if (Input::GetInstance()->TriggerKey(DIK_R)) {
			Reset();
			return;
		}

		Vector3 gravityVector = {0.0f, -Player::GetGravityAcceleration(), 0.0f};
		player_->Update(gravityVector, cameraTargetAngleZ_);

		goal_->Update();

		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}
		for (ChasingEnemy* enemy : chasingEnemies_) {
			enemy->Update();
		}
		for (ShooterEnemy* enemy : shooterEnemies_) {
			enemy->Update();
		}

		cameraController_->Update();
		CheckAllCollisions();
		ChangePhase();
		break;
	}
	case Phase::kDeath:
		deathParticles_->Update();
		if (deathParticles_ && deathParticles_->IsFinished()) {
			phase_ = Phase::kDeathFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}
		break;
	case Phase::kDeathFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			Reset();
		}
		break;
	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	// --- 共通更新 ---

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (worldTransformBlock == nullptr) {
				continue;
			}
			TransformUpdater::WorldTransformUpdate(*worldTransformBlock);
			worldTransformBlock->TransferMatrix();
		}
	}

	skydome_->Update();

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_0)) {
		if (isDebugCameraActive_) {
			isDebugCameraActive_ = false;
		} else {
			isDebugCameraActive_ = true;
		}
	}
#endif

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		Vector3 targetPosition = player_->GetWorldPosition();
		Vector3 baseOffset = cameraController_->targetOffset;
		float currentAngle = camera_.rotation_.z;
		float diff = std::fmod(cameraTargetAngleZ_ - currentAngle + std::numbers::pi_v<float>, 2.0f * std::numbers::pi_v<float>) - std::numbers::pi_v<float>;
		float lerpedAngle = currentAngle + diff * 0.2f;
		camera_.rotation_.z = lerpedAngle;
		Matrix4x4 rollMatrix = TransformUpdater::MakeRoteZMatrix(camera_.rotation_.z);
		Vector3 rotatedOffset = TransformUpdater::TransformNormal(baseOffset, rollMatrix);
		Vector3 rotatedUpVector = TransformUpdater::TransformNormal({0.0f, 1.0f, 0.0f}, rollMatrix);
		Vector3 cameraPosition = targetPosition + rotatedOffset;
		camera_.matView = TransformUpdater::MakeLookAtMatrix(cameraPosition, targetPosition, rotatedUpVector);
		camera_.UpdateProjectionMatrix();
		camera_.TransferMatrix();
	}

	HUD_->Update();
	if (isPaused_) {
		UI_->Update();
	}
}

void GameScene::Draw() {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (worldTransformBlock == nullptr) {
				continue;
			}
			cubeModel_->Draw(*worldTransformBlock, camera_);
		}
	}

	player_->Draw();

	goal_->Draw(camera_);

	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}
	for (ChasingEnemy* enemy : chasingEnemies_) {
		enemy->Draw();
	}
	for (ShooterEnemy* enemy : shooterEnemies_) {
		enemy->Draw();
	}

	if (deathParticles_) {
		deathParticles_->Draw();
	}

	skydome_->Draw();

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();

	// フェード（黒い背景）を描画
	fade_->Draw();

	// ステージ開始時は、フェードの上から文字を表示する
	if (phase_ == Phase::kStageStart) {
		HUD_->DrawStageNumber(currentStageNo_);
	}

	Sprite::PreDraw(dxCommon->GetCommandList());
	if (Input::GetInstance()->PushKey(DIK_J)) {
		jSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
	} else {
		jSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	}
	jSprite_->Draw();
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		spaceSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
	} else {
		spaceSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	}
	spaceSprite_->Draw();
	Sprite::PostDraw();

	HUD_->Draw(player_);
	if (isPaused_) {
		UI_->Draw(pauseMenuIndex_);
	}
}

void GameScene::GenerateBlocks() {
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();
	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

void GameScene::CheckAllCollisions() {
	AABB aabb1, aabb2;

	if (player_->GetIsInvicible() == false) {
		aabb1 = player_->GetAABB();

		for (Enemy* enemy : enemies_) {
			if (player_->GetIsAlive() && enemy->GetIsAlive()) {
				aabb2 = enemy->GetAABB();
				if (IsColliding(aabb1, aabb2)) {
					player_->OnCollision(enemy->GetWorldTransform());
					enemy->OnCollision(player_);
				}
			}
		}

		for (ChasingEnemy* enemy : chasingEnemies_) {
			if (player_->GetIsAlive() && enemy->GetIsAlive()) {
				aabb2 = enemy->GetAABB();
				if (IsColliding(aabb1, aabb2)) {
					player_->OnCollision(enemy->GetWorldTransform());
					enemy->OnCollision(player_);
				}
			}
		}

		for (ShooterEnemy* enemy : shooterEnemies_) {
			if (player_->GetIsAlive() && enemy->GetIsAlive()) {
				aabb2 = enemy->GetAABB();
				if (IsColliding(aabb1, aabb2)) {
					player_->OnCollision(enemy->GetWorldTransform());
					enemy->OnCollision(player_);
				}

				const auto& projectiles = enemy->GetProjectiles();
				for (const auto& projectile : projectiles) {
					if (projectile->IsAlive()) {
						aabb2 = projectile->GetAABB();
						if (IsColliding(aabb1, aabb2)) {
							player_->OnCollision(projectile->GetWorldTransform());
							projectile->OnCollision();
						}
					}
				}
			}
		}
	}

	if (player_->GetIsAttacking() == true) {
		aabb1 = player_->GetAABB();
		for (Enemy* enemy : enemies_) {
			aabb2 = enemy->GetAABB();
			if (IsColliding(aabb1, aabb2)) {
				enemy->SetIsAlive(false);
			}
		}
		for (ChasingEnemy* enemy : chasingEnemies_) {
			aabb2 = enemy->GetAABB();
			if (IsColliding(aabb1, aabb2)) {
				enemy->SetIsAlive(false);
			}
		}
		for (ShooterEnemy* enemy : shooterEnemies_) {
			aabb2 = enemy->GetAABB();
			if (IsColliding(aabb1, aabb2)) {
				enemy->SetIsAlive(false);
			}
		}
	}

	aabb1 = player_->GetAABB();
	aabb2 = goal_->GetAABB();

	if (IsColliding(aabb1, aabb2)) {
		if (phase_ == Phase::kPlay) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}
	}
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		if (!player_->GetIsAlive()) {
			phase_ = Phase::kDeath;
			deathParticles_->Start(player_->GetWorldPosition());
		}
		break;
	case Phase::kDeath:
		break;
	}
}

GameScene::~GameScene() {
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}

	delete cubeModel_;
	delete modelSkydome_;
	delete playerModel_;
	delete goalModel_;
	delete chasingEnemyModel_;
	if (shooterEnemyModel_ && shooterEnemyModel_ != enemyModel_) {
		delete shooterEnemyModel_;
	}
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	for (ChasingEnemy* enemy : chasingEnemies_) {
		delete enemy;
	}
	for (ShooterEnemy* enemy : shooterEnemies_) {
		delete enemy;
	}

	delete player_;
	delete goal_;
	delete skydome_;
	delete debugCamera_;
	delete mapChipField_;
	delete cameraController_;
	delete deathParticles_;
	delete fade_;
	delete HUD_;
	delete UI_;
}