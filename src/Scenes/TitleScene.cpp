// ★必ずincludeより前に書く
#define _USE_MATH_DEFINES

#include "Scenes/TitleScene.h"
#include "Effects/Skydome.h"
#include "Utils/TransformUpdater.h"
#include <cmath>
// std::minなどは使わず math.h の関数を使うため include は最小限

using namespace KamataEngine;

// デストラクタの実装
TitleScene::~TitleScene() {
	delete modelTitle_;
	delete modelPlayer_;
	delete modelEnemy_;
	delete modelShooterEnemy_;
	delete modelChasingEnemy_;
	delete fade_;
	delete skydome_;
}

// 初期化処理
void TitleScene::Initialize() {
	modelTitle_ = KamataEngine::Model::CreateFromOBJ("TitleName2");
	modelPlayer_ = KamataEngine::Model::CreateFromOBJ("AL3_Player");
	modelEnemy_ = KamataEngine::Model::CreateFromOBJ("AL3_Enemy");
	modelShooterEnemy_ = KamataEngine::Model::CreateFromOBJ("AL3_ShooterEnemy");
	modelChasingEnemy_ = KamataEngine::Model::CreateFromOBJ("AL3_ChasingEnemy");
	skydomeModel_ = KamataEngine::Model::CreateFromOBJ("skydome", true);

	skydomeTextureHandle_ = TextureManager::Load("skydome/AL_skysphere.png");

	worldTransformTitle_.Initialize();
	worldTransformTitle_.translation_ = {0.0f, 5.5f, 0.0f};
	worldTransformTitle_.scale_ = {2.0f, 2.0f, 2.0f};
	worldTransformTitle_.rotation_ = {0.0f, 0.0f, 0.0f};

	worldTransformPlayer_.Initialize();
	worldTransformPlayer_.translation_ = {0.0f, 20.0f, 0.0f};
	worldTransformPlayer_.scale_ = {2.0f, 2.0f, 2.0f};
	worldTransformPlayer_.rotation_ = {0.0f, static_cast<float>(M_PI), 0.0f};

	// --- 敵のTransform初期化（4体分） ---
	for (int i = 0; i < 4; ++i) {
		worldTransformEnemies_[i].Initialize();
		worldTransformEnemies_[i].scale_ = {0.0f, 0.0f, 0.0f}; // 最初は非表示
		worldTransformEnemies_[i].rotation_ = worldTransformPlayer_.rotation_;
	}

	playerVelocityY_ = 0.0f;
	isLanding_ = false;
	bounceTimer_ = 0.0f;
	enemyTimer_ = 0.0f;

	camera_.Initialize();
	camera_.translation_ = {0.0f, 2.0f, -15.0f};

	skydome_ = new Skydome();
	skydome_->Initialize(skydomeModel_, skydomeTextureHandle_, &camera_);

	fade_ = new Fade();
	fade_->Initialize();

	objectColorTitle_.Initialize();
	colorTitle_ = {0.14f, 1.0f, 0.6f, 1.0f};

	phase_ = Phase::kFadeIn;
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	finished_ = false;
}

// 更新処理
void TitleScene::Update() {
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// --- 1. タイトル文字のアニメーション ---
		animationTimer_ += 1.0f / 60.0f;
		worldTransformTitle_.translation_.y = initialTitleY_ + std::sin(animationTimer_ * 5.0f) * 0.5f + 5.5f;

		// --- 2. プレイヤーの落下・バウンド処理 ---
		if (!isLanding_) {
			playerVelocityY_ -= 0.020f;
			worldTransformPlayer_.translation_.y += playerVelocityY_;

			if (worldTransformPlayer_.translation_.y <= 0.0f) {
				worldTransformPlayer_.translation_.y = 0.0f;
				playerVelocityY_ = 0.0f;
				isLanding_ = true;
				bounceTimer_ = 0.0f;
			}
		} else {
			if (bounceTimer_ < 1.0f) {
				bounceTimer_ += 1.0f / 60.0f;
				float decay = (1.0f - bounceTimer_);
				float wave = std::sin(bounceTimer_ * 25.0f);
				float scaleDiff = wave * decay * 0.8f;

				worldTransformPlayer_.scale_.y = 2.0f - scaleDiff;
				worldTransformPlayer_.scale_.x = 2.0f + scaleDiff;
				worldTransformPlayer_.scale_.z = 2.0f + scaleDiff;
			} else {
				worldTransformPlayer_.scale_ = {2.0f, 2.0f, 2.0f};
				if (enemyTimer_ < 1.0f) {
					enemyTimer_ += 1.0f / 60.0f;
				}
			}
		}

		// --- 3. 敵の位置更新（4体） ---
		if (bounceTimer_ >= 1.0f) {

			float t = fminf(enemyTimer_, 1.0f);
			float easeT = 1.0f - std::pow(1.0f - t, 3.0f);

			KamataEngine::Vector3 playerPos = worldTransformPlayer_.translation_;
			float startDistance = 20.0f; // 画面外から来るための追加距離

			// ★変更点1：きれいな四角形ではなく、少しバラバラな座標を直接定義
			// 配列の並び：[左下気味, 右下気味, 左上気味, 右上気味] だがズレている

			float dirX[4] = {-8.0f, 5.0f, -4.0f, 9.0f};  // 左右も非対称
			float dirY[4] = {1.0f, 2.5f, 11.0f, 7.5f};   // 高さもバラバラ
			float dirZ[4] = {12.0f, 9.0f, 11.0f, 14.0f}; // 奥行きも少しズラす

			for (int i = 0; i < 4; ++i) {
				worldTransformEnemies_[i].scale_ = {2.0f, 2.0f, 2.0f};

				// 基本の目標位置
				KamataEngine::Vector3 targetPos = playerPos;
				targetPos.x += dirX[i];
				targetPos.y += dirY[i];
				targetPos.z += dirZ[i];

				// ★変更点2：サイン波による浮遊アニメーションを追加
				// animationTimer_ を使って、時間経過で上下させる
				// (float)i を足すことで、4体がタイミングをずらして動く
				float cycle = 2.0f;     // 速さ
				float amplitude = 0.5f; // 揺れ幅
				float hoverY = std::sin(animationTimer_ * cycle + (float)i) * amplitude;

				// 目標位置に浮遊分を足す
				targetPos.y += hoverY;

				// 開始位置（上空斜めから降ってくる）
				KamataEngine::Vector3 startPos = targetPos;
				// ※ targetPos自体が揺れているので、startPosも影響を受けるが、
				// 降りてくる演出としては違和感ないのでこのまま計算
				startPos.x += (dirX[i] > 0 ? startDistance : -startDistance);
				startPos.y += startDistance;

				// イージング移動
				worldTransformEnemies_[i].translation_.x = startPos.x + (targetPos.x - startPos.x) * easeT;
				worldTransformEnemies_[i].translation_.y = startPos.y + (targetPos.y - startPos.y) * easeT;
				worldTransformEnemies_[i].translation_.z = startPos.z + (targetPos.z - startPos.z) * easeT;

				// プレイヤーの方を向く回転計算
				float dx = playerPos.x - worldTransformEnemies_[i].translation_.x;
				float dy = playerPos.y - worldTransformEnemies_[i].translation_.y;
				float dz = playerPos.z - worldTransformEnemies_[i].translation_.z;

				worldTransformEnemies_[i].rotation_.y = std::atan2(dx, dz);

				float horizontalDist = std::sqrt(dx * dx + dz * dz);
				worldTransformEnemies_[i].rotation_.x = std::atan2(-dy, horizontalDist);
			}

		} else {
			// バウンド中は非表示
			for (int i = 0; i < 4; ++i) {
				worldTransformEnemies_[i].scale_ = {0.0f, 0.0f, 0.0f};
			}
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	TransformUpdater::WorldTransformUpdate(worldTransformTitle_);
	TransformUpdater::WorldTransformUpdate(worldTransformPlayer_);

	// 4体分の更新
	for (int i = 0; i < 4; ++i) {
		TransformUpdater::WorldTransformUpdate(worldTransformEnemies_[i]);
	}

	skydome_->Update();
	camera_.UpdateMatrix();
	objectColorTitle_.SetColor(colorTitle_);
}

// 描画処理
void TitleScene::Draw() {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());
	skydome_->Draw();
	KamataEngine::Model::PostDraw();

	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	modelTitle_->Draw(worldTransformTitle_, camera_, &objectColorTitle_);
	modelPlayer_->Draw(worldTransformPlayer_, camera_);

	// 敵の描画（4体分）
	for (int i = 0; i < 4; ++i) {
		KamataEngine::Model* currentModel = modelEnemy_; // デフォルトは通常

		// インデックスによってモデルを切り替え
		if (i == 1)
			currentModel = modelShooterEnemy_; // 2体目はShooter
		if (i == 2)
			currentModel = modelChasingEnemy_; // 3体目はChasing
		if (i == 3)
			currentModel = modelShooterEnemy_; // 4体目もShooter（または通常）

		currentModel->Draw(worldTransformEnemies_[i], camera_);
	}

	KamataEngine::Model::PostDraw();

	fade_->Draw();
}