#include "StageSelectScene.h"
#include "Effects/Fade.h"
#include "Effects/Skydome.h"          // Skydomeクラスを使うために必要
#include "Utils/TransformUpdater.h" // WorldTransformの更新に必要
#include <memory>

using namespace KamataEngine;

// ### 初期化処理 ###
void StageSelectScene::Initialize() {
	camera_.Initialize();
	// カメラを3D空間が見えるように少し後ろに下げる
	camera_.translation_ = {0.0f, 2.0f, -40.0f};

	// --- 3Dモデルの読み込み ---
	// GameSceneで使っているモデルを再利用
	skydomeModel_ = Model::CreateFromOBJ("skydome", true);
	stageCubeModel_ = Model::CreateFromOBJ("debugCube");
	cursorModel_ = Model::CreateFromOBJ("AL3_Player"); // カーソルも同じモデルを流用

	// 1. スカイドーム用のテクスチャを読み込む
	uint32_t skydomeTexture = TextureManager::Load("skydome/AL_skysphere.png");

	// --- 3Dオブジェクトの生成 ---
	// 背景天球
	skydome_ = new Skydome();
	// 2. 正しい引数（モデル、テクスチャ、カメラ）を渡す
	skydome_->Initialize(skydomeModel_, skydomeTexture, &camera_);

	// ステージ選択肢の立方体
	for (int i = 0; i < maxStages_; ++i) {
		int row = i / 5; // 0～4なら0, 5～9なら1
		int col = i % 5; // 0～4の繰り返し

		// 上の段はY=5.0f, 下の段はY=-5.0fに配置
		Vector3 iconPos = {-20.0f + col * 10.0f, 5.0f - row * 10.0f, 0.0f};

		WorldTransform* newTransform = new WorldTransform();
		newTransform->Initialize();
		newTransform->translation_ = iconPos;
		newTransform->scale_ = {2.0f, 2.0f, 2.0f};
		stageCubeTransforms_.push_back(newTransform);
	}

	stageCubeTextureHandles_.resize(maxStages_);

	for (int i = 0; i < maxStages_; ++i) {
		// "stage_icon1.png", "stage_icon2.png", ... というファイル名を生成
		std::string textureName = "debugCube/selectCube" + std::to_string(i + 1) + ".jpg";
		// 読み込んでvectorのi番目に格納
		stageCubeTextureHandles_[i] = TextureManager::Load(textureName);
	}

	// カーソル
	cursorTransform_ = new WorldTransform();
	cursorTransform_->Initialize();
	cursorTransform_->scale_ = {3.0f, 3.0f, 3.0f}; // カーソルはさらに大きく

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// フェーズを初期化し、フェードインを開始
	phase_ = Phase::kFadeIn;
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void StageSelectScene::Update() {
	switch (phase_) {
	case Phase::kFadeIn:
		// フェードイン処理
		fade_->Update();
		// フェードインが終わったらメインフェーズへ
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// 右キー
		if (Input::GetInstance()->TriggerKey(DIK_RIGHT)) {
			cursorCol_++;
			if (cursorCol_ > 4) {
				cursorCol_ = 0;
			}
		}
		// 左キー
		if (Input::GetInstance()->TriggerKey(DIK_LEFT)) {
			cursorCol_--;
			if (cursorCol_ < 0) {
				cursorCol_ = 4;
			}
		}
		// 上下キー
		if (Input::GetInstance()->TriggerKey(DIK_UP) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
			cursorRow_ = 1 - cursorRow_; // 0なら1に、1なら0に切り替える
		}

		// 行と列から最終的なステージ番号を計算
		selectedStageIndex_ = cursorRow_ * 5 + cursorCol_;

		// SPACEキーが押されたらフェードアウトを開始
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, 1.0f); // 1秒でフェードアウト
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		// フェードアウト処理
		fade_->Update();
		// フェードアウトが終わったらシーンを終了
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	// カーソルの3D座標を更新する
	// 選択されている立方体の上あたりに配置
	Vector3 selectedCubePos = stageCubeTransforms_[selectedStageIndex_]->translation_;
	cursorTransform_->translation_ = {selectedCubePos.x, selectedCubePos.y + 4.0f, selectedCubePos.z};

	// カーソルをクルクル回転させる演出 (変更なし)
	cursorTransform_->rotation_.y += 0.05f;

	// 全てのWorldTransformの行列を更新 (変更なし)
	for (WorldTransform* transform : stageCubeTransforms_) {
		TransformUpdater::WorldTransformUpdate(*transform);
		transform->TransferMatrix();
	}
	TransformUpdater::WorldTransformUpdate(*cursorTransform_);
	cursorTransform_->TransferMatrix();

	skydome_->Update();
	camera_.UpdateMatrix();
}

void StageSelectScene::Draw() {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画の開始
	Model::PreDraw(dxCommon->GetCommandList());

	// 天球の描画
	skydome_->Draw();

	// 3Dモデル描画の終了
	Model::PostDraw();
	// ステージ選択肢の描画
	for (int i = 0; i < maxStages_; ++i) {
		// 3Dモデル描画の開始
		Model::PreDraw(dxCommon->GetCommandList());
		// i番目のTransformを取得
		WorldTransform* transform = stageCubeTransforms_[i];
		// i番目のテクスチャハンドルを取得
		uint32_t textureHandle = stageCubeTextureHandles_[i];

		// 取得したTransformとテクスチャハンドルを使って描画
		stageCubeModel_->Draw(*transform, camera_, textureHandle);
		// 3Dモデル描画の終了
		Model::PostDraw();
	}
	// 3Dモデル描画の開始
	Model::PreDraw(dxCommon->GetCommandList());

	// カーソルの描画
	cursorModel_->Draw(*cursorTransform_, camera_);

	// フェードの描画 (必ず一番最後に描画する)
	fade_->Draw();

	// 3Dモデル描画の終了
	Model::PostDraw();
}

StageSelectScene::~StageSelectScene() {
	// モデルの解放
	delete skydomeModel_;
	delete stageCubeModel_;
	// cursorModel_はstageCubeModel_と同じものを指しているので二重解放しない

	// オブジェクトの解放
	delete skydome_;
	for (WorldTransform* transform : stageCubeTransforms_) {
		delete transform;
	}
	stageCubeTransforms_.clear();
	delete cursorTransform_;

	// フェードの解放
	delete fade_;
}