#include "GameScene.h"
#include "Effects/DeathParticles.h"
#include "Effects/Fade.h"
#include "Effects/Skydome.h"
#include "Objects/Enemy.h"
#include "Objects/Goal.h"
#include "Objects/Player.h"
#include "System/CameraController.h"
#include "System/MapChipField.h"
#include "Utils/TransformUpdater.h"
#include <Windows.h> // OutputDebugStringA を使うために必要
#include <cstdio>    // sprintf_s を使うために必要

using namespace KamataEngine;

/// <summary>
/// AABB同士の交差判定
/// </summary>
/// <param name="aabb1">AABB1</param>
/// <param name="aabb2">AABB2</param>
/// <returns>交差しているか</returns>
bool IsColliding(const AABB& aabb1, const AABB& aabb2) {
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) && (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) && (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z)) {
		return true;
	}
	return false;
}

void GameScene::Reset() {
	// プレイヤーの初期化
	// プレイヤーの初期座標をマップから取得
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 9);
	player_->Initialize(playerModel_, playerTextureHandle_, &camera_, playerPosition);
	player_->SetMapChipField(mapChipField_); // マップチップフィールドを再設定

	// 敵の初期化
	// まず現在の敵を全て削除
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	// 再度、敵を生成・初期化
	const int kNumEnemies = 1;
	for (int i = 0; i < kNumEnemies; ++i) {
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(10 + i * 5, 9);
		Enemy* newEnemy = new Enemy();
		newEnemy->Initialize(enemyModel_, enemyTextureHandle_, &camera_, enemyPosition);
		newEnemy->SetMapChipField(mapChipField_);
		enemies_.push_back(newEnemy);
	}

	// カメラコントローラーのリセット
	cameraController_->SetTarget(player_);
	cameraController_->Reset();
	cameraTargetAngleZ_ = 0.0f; // カメラの目標角度もリセット

	// ゴールの初期化
	Vector3 goalPosition = mapChipField_->GetMapChipPositionByIndex(5, 4);
	goal_->Initialize(goalModel_, goalPosition);

	// フェーズをフェードインに戻す
	phase_ = Phase::kFadeIn;
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void GameScene::Initialize(int stageNo) {
	// テクスチャの読み込み
	uvCheckerTextureHandle_ = TextureManager::Load("uvChecker.png");
	playerTextureHandle_ = TextureManager::Load("AL3_Player/Player.png");
	enemyTextureHandle_ = TextureManager::Load("AL3_Enemy/Enemy.png");
	skysphereTextureHandle = TextureManager::Load("skydome/AL_skysphere.png");
	particleTextureHandle = TextureManager::Load("AL3_Particle/AL3_Particle.png");

	// 3Dモデルの生成
	cubeModel_ = Model::CreateFromOBJ("debugCube");
	// 3Dモデルの生成
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	// 3Dモデルの生成
	playerModel_ = Model::CreateFromOBJ("AL3_Player", true);
	// 3Dモデルの生成
	enemyModel_ = Model::CreateFromOBJ("AL3_Enemy", true);
	// パーティクルのモデル生成
	particleModel_ = Model::CreateFromOBJ("AL3_Particle", true);
	// ゴールのモデル生成
	goalModel_ = Model::CreateFromOBJ("AL3_Particle", true);

	// 座標変換の初期化
	worldTransform_.Initialize();
	// カメラの初期化
	camera_.Initialize();
	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// マップチップフィールドの生成を先に移動させる
	mapChipField_ = new MapChipField();
	// ステージ番号に応じて読み込むファイル名を作成
	std::string mapFileName = "Resources/stage/stage" + std::to_string(stageNo) + ".csv";
	mapChipField_->LoadMapChipCsv(mapFileName);

	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 9);
	switch (stageNo) {
	case 1:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 10);
		break;
	case 2:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 17);
		break;
	case 3:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	case 4:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	case 5:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	case 6:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	case 7:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	case 8:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	case 9:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	case 10:
		playerPosition = mapChipField_->GetMapChipPositionByIndex(3, 17);
		break;
	}

	// 自キャラの生成
	player_ = new Player();
	// 自キャラの初期化
	player_->Initialize(playerModel_, playerTextureHandle_, &camera_, playerPosition);

	// デスパーティクルの生成（テスト）
	deathParticles_ = new DeathParticles();
	// デスパーティクルの初期化
	// モデルはキューブ、テクスチャはuvChecker、座標は自キャラと同じものを仮で使う
	deathParticles_->Initialize(particleModel_, particleTextureHandle, &camera_, playerPosition);

	// 敵を複数生成する
	const int kNumEnemies = 1; // 生成したい敵の数
	for (int i = 0; i < kNumEnemies; ++i) {
		// 敵の初期座標を設定 (ここでは例としてX座標をずらして配置)
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(10 + i * 5, 9);

		// 敵キャラの生成
		Enemy* newEnemy = new Enemy();
		// 敵キャラの初期化
		newEnemy->Initialize(enemyModel_, enemyTextureHandle_, &camera_, enemyPosition);
		newEnemy->SetMapChipField(mapChipField_);
		// リストに追加
		enemies_.push_back(newEnemy);
	}

	// 要素数
	const uint32_t kNumblockVirtical = 12;
	const uint32_t kNumBlockHorizontal = 12;

	// 要素数を変更する
	// 列数を指定
	worldTransformBlocks_.resize(kNumblockVirtical);
	for (uint32_t i = 0; i < kNumblockVirtical; ++i) {
		// 1列の要素数を設定(横方向のブロック数)
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	// 天球の生成
	skydome_ = new Skydome();
	// 天球の初期化
	skydome_->Initialize(modelSkydome_, skysphereTextureHandle, &camera_);

	// プレイヤーにマップチップフィールドをセット
	player_->SetMapChipField(mapChipField_);

	// ゴールの生成と初期化
	goal_ = new Goal();
	// ゴールの位置をマップチップから取得（例: 98行, 18列目）
	Vector3 goalPosition = mapChipField_->GetMapChipPositionByIndex(5, 4);
	goal_->Initialize(goalModel_, goalPosition);

	// ブロックの生成処理
	GenerateBlocks();

	// カメラコントローラーの生成
	cameraController_ = new CameraController(); //
	// カメラコントローラーの初期化
	cameraController_->Initialize(&camera_); //
	// 追従対象をセット
	cameraController_->SetTarget(player_); //
	// リセット（瞬間合わせ）
	cameraController_->Reset(); //

	// カメラの移動範囲の指定
	Rect cameraMovableArea;
	cameraMovableArea.left = 0.0f;                        // 例: 左端の座標
	cameraMovableArea.right = 200.0f;                     // 例: 右端の座標
	cameraMovableArea.bottom = 0.0f;                      // 例: 下端の座標
	cameraMovableArea.top = 100.0f;                       // 例: 上端の座標
	cameraController_->SetMovableArea(cameraMovableArea); // カメラコントローラーに設定

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// フェードインから開始
	fade_->Start(Fade::Status::FadeIn, 1.0f); // 1秒でフェードイン

	// ゲームプレイフェーズから開始
	phase_ = Phase::kFadeIn;

	// 終了フラグを初期化
	finished_ = false;
}

void GameScene::Update() {

	// --- フェーズごとの処理 ---
	switch (phase_) {
	case Phase::kFadeIn: // フェードイン処理
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kPlay;
		}
		cameraController_->Update();
		break;

	case Phase::kPlay: {

		// リセット処理を追加
		if (Input::GetInstance()->TriggerKey(DIK_R)) {
			Reset();
			// Reset関数内でフェードインが開始されるので、すぐにreturnする
			return;
		}

		// --- ステップ3: 決定した重力方向から重力ベクトルを生成 ---
		Vector3 gravityVector = {0.0f, -Player::GetGravityAcceleration(), 0.0f};

		// プレイヤーの更新（計算した重力ベクトルを渡す）
		player_->Update(gravityVector, cameraTargetAngleZ_);

		goal_->Update();

		for (Enemy* enemy : enemies_) {
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
			// シーン終了ではなくフェードアウトへ移行
			phase_ = Phase::kDeathFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f); // 1秒でフェードアウト
		}
		break;
	case Phase::kDeathFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			Reset(); // フェードアウトが終わったらリセット
		}
		break;
	case Phase::kFadeOut: // フェードアウト処理
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	// --- 以下は両方のフェーズで共通して行う処理 ---

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (worldTransformBlock == nullptr) {
				continue;
			}
			TransformUpdater::WorldTransformUpdate(*worldTransformBlock);
			worldTransformBlock->TransferMatrix();
		}
	}

	// 天球の更新
	skydome_->Update();
// --- デバッグカメラ関連の処理（変更なし） ---
#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_0)) {
		if (isDebugCameraActive_) {
			isDebugCameraActive_ = false;
		} else {
			isDebugCameraActive_ = true;
		}
	}
#endif // _DEBUG

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		// --- 軌道カメラ方式による疑似回転 ---

		// 1. 追従対象（プレイヤー）のワールド座標を取得
		Vector3 targetPosition = player_->GetWorldPosition();

		// 2. プレイヤーからのカメラの基本オフセットを定義
		Vector3 baseOffset = cameraController_->targetOffset; // 少し上、後ろに配置

		// 3. カメラの目標角度（ロール回転）に向かって滑らかに補間
		float currentAngle = camera_.rotation_.z;
		float diff = std::fmod(cameraTargetAngleZ_ - currentAngle + std::numbers::pi_v<float>, 2.0f * std::numbers::pi_v<float>) - std::numbers::pi_v<float>;
		float lerpedAngle = currentAngle + diff * 0.2f;
		camera_.rotation_.z = lerpedAngle; // 現在の角度として保存しておく

		// 4. ロール回転用の行列を作成
		Matrix4x4 rollMatrix = TransformUpdater::MakeRoteZMatrix(camera_.rotation_.z);

		// 5. カメラのオフセットと「上方向」ベクトルを回転させる
		Vector3 rotatedOffset = TransformUpdater::TransformNormal(baseOffset, rollMatrix);
		Vector3 rotatedUpVector = TransformUpdater::TransformNormal({0.0f, 1.0f, 0.0f}, rollMatrix);

		// 6. 最終的なカメラの座標を計算
		Vector3 cameraPosition = targetPosition + rotatedOffset;

		// 7. 新しいLookAt関数を使ってビュー行列を直接設定
		camera_.matView = TransformUpdater::MakeLookAtMatrix(cameraPosition, targetPosition, rotatedUpVector);
		// プロジェクション行列はカメラ内部のものを使う
		camera_.UpdateProjectionMatrix();

		camera_.TransferMatrix();
	}
}

void GameScene::Draw() {
	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	// ブロックの描画
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

	// デスパーティクルの描画
	if (deathParticles_) {
		deathParticles_->Draw();
	}

	// 自キャラの描画
	skydome_->Draw();

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();

	// フェードの描画
	fade_->Draw();
}

void GameScene::GenerateBlocks() {
	// 要素数
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	// 要素数を変更する
	// 列数を設定(縦方向のブロック数)
	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		// 1列の要素数を設定(横方向のブロック数)
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
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
	// 判定対象AとBのAABB
	AABB aabb1, aabb2;

#pragma region 自キャラと敵キャラの当たり判定
	if (player_->GetIsInvicible() == false) {
		// 自キャラのAABBを取得
		aabb1 = player_->GetAABB();

		// 自キャラと敵全ての当たり判定
		for (Enemy* enemy : enemies_) {
			if (player_->GetIsAlive() && enemy->GetIsAlive()) {
				// 敵のAABBを取得
				aabb2 = enemy->GetAABB();

				// AABB同士の交差判定
				if (IsColliding(aabb1, aabb2)) {
					// 衝突応答
					player_->OnCollision(enemy); //
					enemy->OnCollision(player_); //
				}
			}
		}
	}
#pragma endregion

#pragma region 攻撃状態の自キャラと敵キャラの当たり判定
	if (player_->GetIsAttacking() == true) {
		// 自キャラのAABBを取得
		aabb1 = player_->GetAABB();

		// 自キャラと敵全ての当たり判定
		for (Enemy* enemy : enemies_) {
			// 敵のAABBを取得
			aabb2 = enemy->GetAABB();

			// AABB同士の交差判定
			if (IsColliding(aabb1, aabb2)) {
				// 衝突応答
				enemy->SetIsAlive(false); //
			}
		}
	}
#pragma endregion

#pragma region 自キャラとゴールの当たり判定
	// 自キャラのAABBを取得
	aabb1 = player_->GetAABB();
	// ゴールのAABBを取得
	aabb2 = goal_->GetAABB();

	// AABB同士の交差判定
	if (IsColliding(aabb1, aabb2)) {
		// ゴールしたらフェードアウトを開始する
		if (phase_ == Phase::kPlay) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f); // 1秒でフェードアウト
		}
	}
#pragma endregion
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		// ゲームプレイフェーズの切り替え処理
		// 条件：自キャラがデス状態になったら
		if (!player_->GetIsAlive()) {
			// 処理１：死亡演出フェーズに切り替え
			phase_ = Phase::kDeath;
			// 処理２：自キャラの座標にデスパーティクルを発生、初期化
			deathParticles_->Start(player_->GetWorldPosition());
		}
		break;

	case Phase::kDeath:
		// デス演出フェーズの切り替え処理
		// (今回は何もしない)
		break;
	}
}

GameScene::~GameScene() {
	// ブロック用のWorldTransformを解放
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}

	// 各種モデルを解放
	delete cubeModel_;
	delete modelSkydome_;
	delete playerModel_;
	delete goalModel_;
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}

	// 各オブジェクトを解放
	delete player_;
	delete goal_;
	delete skydome_;
	delete debugCamera_;
	delete mapChipField_;
	delete cameraController_;
	delete deathParticles_;
	delete fade_;
}
