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
#include "System/Gamepad.h"
#include "System/MapChipField.h"
#include "UI/UI.h"
#include "Utils/Easing.h"
#include "Utils/TransformUpdater.h"
#include <Windows.h>
#include <algorithm>
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

	isPaused_ = false;
	// --- 1. プレイヤーの再配置と初期化 ---
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 9); // フォールバック

	MapChipField::IndexSet startIdx;
	if (mapChipField_->FindFirstIndexByType(MapChipType::kPlayerStart, startIdx)) {
		playerPosition = mapChipField_->GetMapChipPositionByIndex(startIdx.xIndex, startIdx.yIndex);
	}

	// プレイヤーの状態をリセット（Initialize済みのインスタンスを再設定）
	player_->Initialize(playerModel_, playerTextureHandle_, swordModel_, swordTextureHandle_, &camera_, playerPosition);
	player_->SetMapChipField(mapChipField_);

	// --- 2. 敵の全削除と再生成 ---
	// 既存の敵を削除
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	for (ChasingEnemy* enemy : chasingEnemies_) {
		delete enemy;
	}
	chasingEnemies_.clear();

	for (ShooterEnemy* enemy : shooterEnemies_) {
		delete enemy;
	}
	shooterEnemies_.clear();

	// マップから敵を生成
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVertical(); ++y) {
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

	// --- 3. カメラとゴールのリセット ---
	// ゴール位置の再取得と設定（マップチップにゴールがあればそれを優先）
	Vector3 goalPosition;

	// 全マップを走査して kGoal を探し、見つかった中で「最も右側(x 最大)」を採用する
	bool foundGoal = false;
	MapChipField::IndexSet bestGoalIdx = {0, 0};
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVertical(); ++y) {
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x) {
			if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kGoal) {
				if (!foundGoal || x > bestGoalIdx.xIndex || (x == bestGoalIdx.xIndex && y > bestGoalIdx.yIndex)) {
					bestGoalIdx.xIndex = x;
					bestGoalIdx.yIndex = y;
					foundGoal = true;
				}
			}
		}
	}

	if (foundGoal) {
		goalPosition = mapChipField_->GetMapChipPositionByIndex(bestGoalIdx.xIndex, bestGoalIdx.yIndex);
		// デバッグ出力
		{
			std::string msg = "Map goal found at index x=" + std::to_string(bestGoalIdx.xIndex) + ", y=" + std::to_string(bestGoalIdx.yIndex) + "\n";
			OutputDebugStringA(msg.c_str());
			std::string posMsg = "Map goal world pos x=" + std::to_string(goalPosition.x) + ", y=" + std::to_string(goalPosition.y) + "\n";
			OutputDebugStringA(posMsg.c_str());
		}
	} else {
		// 互換性のために従来のフォールバック（ステージによる固定位置）を残す
		switch (currentStageNo_) {
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
		// デバッグ出力（フォールバック使用）
		{
			std::string msg = "No map goal found: using fallback for stage " + std::to_string(currentStageNo_) + "\n";
			OutputDebugStringA(msg.c_str());
		}
	}

	// ゴールインスタンスがあれば再初期化（位置のみ更新）
	if (goal_) {
		goal_->Initialize(goalModel_, goalPosition);
	}

	// カメラコントローラーのリセット
	cameraController_->SetTarget(player_);
	cameraController_->Reset();
	cameraTargetAngleZ_ = 0.0f;
	cameraController_->targetOffset = {0, 0, -30.0f}; // カメラ距離の初期化

	// --- 4. フェーズと演出のリセット ---
	// デフォルトではリトライ用のフェードイン
	phase_ = Phase::kFadeIn;
	stageStartTimer_ = 0.0f;

	// FadeIn開始状態（真っ黒）にする
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void GameScene::Initialize(int stageNo) {
	// ★ステージ番号を保存
	currentStageNo_ = stageNo;

	// --- 1. リソースのロード ---
	uvCheckerTextureHandle_ = TextureManager::Load("uvChecker.png");
	playerTextureHandle_ = TextureManager::Load("AL3_Player/Player.png");
	enemyTextureHandle_ = TextureManager::Load("AL3_Enemy/Enemy.png");
	chasingEnemyTextureHandle_ = TextureManager::Load("AL3_ChasingEnemy/ChasingEnemy.png");
	shooterEnemyTextureHandle_ = TextureManager::Load("AL3_ShooterEnemy/ShooterEnemy.png");
	skysphereTextureHandle = TextureManager::Load("skyDome/AL_skySphere.png");
	particleTextureHandle = TextureManager::Load("AL3_Particle/AL3_Particle.png");
	projectileTextureHandle_ = TextureManager::Load("ball/ball.png");
	swordTextureHandle_ = TextureManager::Load("sword/sword.png");

	cubeModel_ = Model::CreateFromOBJ("debugCube");
	modelSkydome_ = Model::CreateFromOBJ("skyDome", true);
	playerModel_ = Model::CreateFromOBJ("AL3_Player", true);
	enemyModel_ = Model::CreateFromOBJ("AL3_Enemy", true);
	chasingEnemyModel_ = Model::CreateFromOBJ("AL3_ChasingEnemy", true);
	shooterEnemyModel_ = Model::CreateFromOBJ("AL3_ShooterEnemy", true);
	particleModel_ = Model::CreateFromOBJ("AL3_Particle", true);
	goalModel_ = Model::CreateFromOBJ("goal", true);
	projectileModel_ = Model::CreateFromOBJ("ball", true);
	swordModel_ = Model::CreateFromOBJ("sword", true);
	// フォルダ名 "clearText" を指定（その中の clearText.obj が読み込まれます）
	clearModel_ = Model::CreateFromOBJ("clearText", true);
	objectColorClear_.Initialize();
	colorClear_ = {3.0f, 3.0f, 0.0f, 1.0f};

	// ロードに失敗していないかログを出すと確実です 🔍
	if (!clearModel_) {
		OutputDebugStringA("Error: clearModel_ のロードに失敗しました！パスを確認してください。\n");
	}

	clearWorldTransform_.Initialize();

	jHandle_ = TextureManager::GetInstance()->Load("HUD/J.png");
	spaceHandle_ = TextureManager::GetInstance()->Load("HUD/space.png");
	escHandle_ = TextureManager::GetInstance()->Load("HUD/esc.png");

	// --- コントローラ用スプライトをロード ---
	aHandle_ = TextureManager::GetInstance()->Load("HUD/A.png");
	xHandle_ = TextureManager::GetInstance()->Load("HUD/X.png");
	selectHandle_ = TextureManager::GetInstance()->Load("HUD/select.png");

	// --- 2. 座標変換・カメラの基本初期化 ---
	worldTransform_.Initialize();
	camera_.Initialize();
	debugCamera_ = new DebugCamera(1280, 720);

	// --- 3. マップの生成 ---
	mapChipField_ = new MapChipField();
	std::string mapFileName = "Resources/stage/stage" + std::to_string(stageNo) + ".csv";
	mapChipField_->LoadMapChipCsv(mapFileName);

	// --- 4. オブジェクトのインスタンス生成 (配置はResetで行う) ---
	player_ = new Player(); // 中身はResetで初期化される

	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(particleModel_, particleTextureHandle, &camera_, {0, 0, 0});

	goal_ = new Goal(); // 中身はResetで初期化される

	// ブロック生成 (マップ依存なのでここで生成)
	GenerateBlocks();

	// 天球
	skydome_ = new Skydome();
	skydome_->Initialize(modelSkydome_, skysphereTextureHandle, &camera_);

	// カメラコントローラー
	cameraController_ = new CameraController();
	cameraController_->Initialize(&camera_);

	Rect cameraMovableArea;
	cameraMovableArea.left = 0.0f;
	cameraMovableArea.right = 200.0f;
	cameraMovableArea.bottom = 0.0f;
	cameraMovableArea.top = 100.0f;
	cameraController_->SetMovableArea(cameraMovableArea);

	// フェード
	fade_ = new Fade();
	fade_->Initialize();

	// UI・HUD
	UI_ = new UI;
	UI_->Initialize();

	HUD_ = new HUD;
	HUD_->Initialize();

	jSprite_ = Sprite::Create(jHandle_, {64, 600});
	if (jSprite_) {
		jSprite_->SetSize({64, 64});
	}
	spaceSprite_ = Sprite::Create(spaceHandle_, {192, 600});
	if (spaceSprite_) {
		spaceSprite_->SetSize({224, 64});
	}
	escSprite_ = Sprite::Create(escHandle_, {64, 128});
	if (escSprite_) {
		escSprite_->SetSize({144, 64});
	}

	// コントローラ用スプライト生成
	if (aHandle_)
		aSprite_ = Sprite::Create(aHandle_, {64, 600});
	if (xHandle_)
		xSprite_ = Sprite::Create(xHandle_, {64, 600});
	if (selectHandle_)
		selectSprite_ = Sprite::Create(selectHandle_, {64, 128});
	if (aSprite_)
		aSprite_->SetSize({64, 64});
	if (xSprite_)
		xSprite_->SetSize({64, 64});
	if (selectSprite_)
		selectSprite_->SetSize({64, 64});

	// 初期入力はキーボードと見なす
	lastInputIsGamepad_ = false;

	finished_ = false;

	// --- 5. 共通初期化処理の呼び出し ---
	// 敵の生成、プレイヤー・カメラの配置リセットなどを実行
	Reset();

	// --- 6. Initialize特有の上書き設定 ---
	// Resetではリトライ用に FadeIn になるが、
	// ステージ開始時は「ステージ番号表示演出」を行いたいので上書きする
	phase_ = Phase::kStageStart;
}

void GameScene::Update() {

	// --- 最終入力デバイス判定（毎フレーム） ---
	{
		Gamepad* gp = Gamepad::GetInstance();
		bool gpActive = false;
		if (gp) {
			gpActive = gp->IsPressed(XINPUT_GAMEPAD_A) || gp->IsPressed(XINPUT_GAMEPAD_B) || gp->IsPressed(XINPUT_GAMEPAD_X) || gp->IsPressed(XINPUT_GAMEPAD_Y) || gp->IsPressed(XINPUT_GAMEPAD_BACK) ||
			           gp->IsPressed(XINPUT_GAMEPAD_START) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_UP) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_DOWN) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) ||
			           gp->IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) || std::abs(gp->GetLeftThumbXf()) > kGamepadStickThreshold || std::abs(gp->GetLeftThumbYf()) > kGamepadStickThreshold;
		}
		bool kbActive = false;
		Input* in = Input::GetInstance();
		if (in) {
			const int keysToCheck[] = {DIK_LEFT, DIK_RIGHT, DIK_UP, DIK_DOWN, DIK_SPACE, DIK_J, DIK_R, DIK_ESCAPE, DIK_RETURN, DIK_A, DIK_D, DIK_W, DIK_S};
			for (int k : keysToCheck) {
				if (in->PushKey(static_cast<unsigned char>(k))) {
					kbActive = true;
					break;
				}
			}
		}
		// 最近操作された方を優先
		if (gpActive && !lastInputIsGamepad_)
			lastInputIsGamepad_ = true;
		else if (kbActive && lastInputIsGamepad_)
			lastInputIsGamepad_ = false;
	}

	// 1. ポーズのトグル判定 (現状のままでOK)
	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		isPaused_ = !isPaused_;
	}

	// 2. ポーズ中の処理を独立させる
	if (isPaused_) {
		bool confirm = false;
		bool cancel = false;
		UI_->HandlePauseInput(pauseMenuIndex_, confirm, cancel);

		if (confirm) {
			if (pauseMenuIndex_ == 0) {
				Reset();
				isPaused_ = false;
				return;
			}
			if (pauseMenuIndex_ == 1) {
				finished_ = true;
			}
		}
		if (cancel) {
			isPaused_ = false;
		}

		return; // ポーズ中は以下のゲームロジック（フェーズ処理）をスキップ
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
			// UI にポーズ入力を処理させる
			bool confirm = false;
			bool cancel = false;
			UI_->HandlePauseInput(pauseMenuIndex_, confirm, cancel);

			if (confirm) {
				if (pauseMenuIndex_ == 0) {
					Reset();
					isPaused_ = false;
					showControls_ = false;
					return;
				} else if (pauseMenuIndex_ == 1) {
					finished_ = true;
				}
			}
			if (cancel) {
				// キャンセル（ESC 相当）はポーズ解除
				isPaused_ = false;
				showControls_ = false;
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

		// タイムスケールの計算
		float timeScale = 1.0f; // 通常は1倍速
		if (player_->GetIsDeadAnimating()) {
			timeScale = 0.2f; // 死亡演出中は0.2倍速（スローモーション）
		}

		// ★変更: timeScale と3種の敵リストを渡す
		player_->Update(gravityVector, cameraTargetAngleZ_, enemies_, chasingEnemies_, shooterEnemies_, timeScale);

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
	case Phase::kGoalAnimation:
		// プレイヤーの演出更新（重力などは演出内で制御）
		player_->UpdateGoalAnimation();

		// --- クリアモデルの「奥から手前」演出更新 ---
		{
			// 演出の時間を 1.0秒 と定義
			float animationDuration = 1.0f;
			// 進捗率 0.0 ～ 1.0
			float t = std::clamp(goalCameraTimer_ / animationDuration, 0.0f, 1.0f);

			// ★ポイント1：EaseOutBack を使って「弾む」ような動きにする
			float easedT = EaseOutBack(t);

			Vector3 playerPos = player_->GetWorldPosition();

			// ★ポイント2：スケールを 0.0 から 1.0 へ（飛び出す感）
			clearWorldTransform_.scale_ = {easedT, easedT, easedT};

			// ★ポイント3：Z軸の移動（奥から手前へ）
			// プレイヤーの位置を 0 とすると、奥(10.0f) から 手前(-5.0f) くらいまで移動
			float startZ = 10.0f;
			float endZ = -5.0f;
			float currentZ = playerPos.z + Lerp(startZ, endZ, easedT);

			// 座標設定（Y座標はプレイヤーの少し上）
			clearWorldTransform_.translation_ = {playerPos.x, playerPos.y + 1.0f, currentZ};

			// 正面を向かせる
			clearWorldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};

			// 行列更新
			TransformUpdater::WorldTransformUpdate(clearWorldTransform_);
			clearWorldTransform_.TransferMatrix();
		}

		// カメラを滑らかに近づける処理 (線形補間)
		{
			goalCameraTimer_ += 1.0f / 60.0f;

			// 0.5秒かけて近づく
			float duration = 0.5f;
			float t = std::clamp(goalCameraTimer_ / duration, 0.0f, 1.0f);

			// Z軸のオフセットを変化させる
			// -30.0f (通常) → -10.0f (アップ)
			float startZ = -30.0f;
			float endZ = -10.0f;

			// Lerpで補間 (t をそのまま使うと線形補間になります)
			cameraController_->targetOffset.z = Lerp(startZ, endZ, t);

			// 必要に応じて高さ(Y)も調整したい場合はここに追加
			// cameraController_->targetOffset.y = Lerp(0.0f, 2.0f, t);
		}

		// カメラはプレイヤーを追い続ける
		cameraController_->Update();

		// プレイヤーの演出が終わったらフェードアウトへ
		if (player_->GetGoalAnimationPhase() == GoalAnimationPhase::kEnd) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}
		break;
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

	objectColorClear_.SetColor(colorClear_);
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

	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	if (phase_ == Phase::kGoalAnimation) {
		clearModel_->Draw(clearWorldTransform_, camera_, &objectColorClear_);
	}

	KamataEngine::Model::PostDraw();

	// フェード（黒い背景）を描画
	fade_->Draw();

	// ステージ開始時は、フェードの上から文字を表示する
	if (phase_ == Phase::kStageStart) {
		HUD_->DrawStageNumber(currentStageNo_);
	}

	Sprite::PreDraw(dxCommon->GetCommandList());
	if (!lastInputIsGamepad_) {
		// キーボード表示（既存）
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
		if (Input::GetInstance()->PushKey(DIK_ESCAPE)) {
			escSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
		} else {
			escSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		}
		escSprite_->Draw();
	} else {
		// コントローラ表示
		// A (ジャンプ/決定)
		if (Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_A)) {
			aSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
		} else {
			aSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		}
		aSprite_->SetPosition({64, 600});
		aSprite_->Draw();

		// X (攻撃)
		if (Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_X)) {
			xSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
		} else {
			xSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		}
		xSprite_->SetPosition({192, 600});
		xSprite_->Draw();

		// START / SELECT 表示（select.png）
		if (Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_START) || Gamepad::GetInstance()->IsPressed(XINPUT_GAMEPAD_BACK)) {
			selectSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
		} else {
			selectSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		}
		selectSprite_->SetPosition({64, 128});
		selectSprite_->Draw();
	}
	Sprite::PostDraw();

	HUD_->Draw(player_);
	if (isPaused_) {
		UI_->Draw(pauseMenuIndex_);
	}
}

void GameScene::GenerateBlocks() {
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVertical();
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

	if (player_->GetIsInvincible() == false) {
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

	// 修正: 死亡演出中や既に死亡状態のときはゴール判定を無視する
	if (IsColliding(aabb1, aabb2)) {
		if (phase_ == Phase::kPlay && player_->GetIsAlive() && !player_->GetIsDeadAnimating()) {
			phase_ = Phase::kGoalAnimation;
			player_->StartGoalAnimation();
			// カメラ演出用タイマーのリセット
			goalCameraTimer_ = 0.0f;
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

	delete clearModel_;
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