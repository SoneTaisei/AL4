#include "Scenes/TitleScene.h"

#include "Utils/TransformUpdater.h"
#include "Effects/Skydome.h"

using namespace KamataEngine;

// デストラクタの実装
TitleScene::~TitleScene() {
	// 各インスタンスを解放
	delete modelTitle_;
	delete modelPlayer_;
	delete fade_;
	delete skydome_;
}

// 初期化処理
void TitleScene::Initialize() {
	// 3Dモデルを読み込む
	// 前回作成したファイル名 "GoodGame.obj" を拡張子なしで指定
	modelTitle_ = KamataEngine::Model::CreateFromOBJ("TitleName2");
	modelPlayer_ = KamataEngine::Model::CreateFromOBJ("AL3_Player");
	skydomeModel_ = KamataEngine::Model::CreateFromOBJ("skydome", true);

	skydomeTextureHandle_ = TextureManager::Load("skydome/AL_skysphere.png");

	// --- タイトル文字のTransform初期化 ---
	worldTransformTitle_.Initialize();
	worldTransformTitle_.translation_ = {0.0f, 5.5f, 0.0f};
	worldTransformTitle_.scale_ = {2.0f, 2.0f, 2.0f};
	worldTransformTitle_.rotation_ = {0.0f, 0.0f, 0.0f};

	// --- プレイヤーのTransform初期化 ---
	worldTransformPlayer_.Initialize();
	worldTransformPlayer_.translation_ = {0.0f, 0.0f, 0.0f}; // タイトルの下に表示
	worldTransformPlayer_.scale_ = {2.0f, 2.0f, 2.0f};
	worldTransformPlayer_.rotation_ = {0.0f, -2.5f, 0.0f};

	// --- カメラの初期化 ---
	camera_.Initialize();
	camera_.translation_ = {0.0f, 2.0f, -15.0f};

	skydome_ = new Skydome();
	skydome_->Initialize(skydomeModel_, skydomeTextureHandle_, &camera_);

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// 色変更オブジェクトの初期化
	objectColorTitle_.Initialize();
	// 色の初期値を設定 (RGBA = {1, 1, 1, 1})
	colorTitle_ = {0.14f, 1.0f, 0.6f, 1.0f};

	// フェーズを初期化し、フェードインを開始
	phase_ = Phase::kFadeIn;
	fade_->Start(Fade::Status::FadeIn, 1.0f); // 1秒でフェードイン

	// 終了フラグを初期化
	finished_ = false;
}

// 更新処理
void TitleScene::Update() {
	// フェーズごとの処理
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
		// メイン処理 (タイトルロゴの上下アニメーション)
		// アニメーション用カウンタを進める
		animationTimer_ += 1.0f / 60.0f; // 1秒間に1/60ずつ増加 (フレームレートを想定)

		// サインカーブを使ってY座標を更新 (振幅: 0.5f, 周期を調整)
		worldTransformTitle_.translation_.y = initialTitleY_ + std::sin(animationTimer_ * 5.0f) * 0.5f+5.5f;

		// スペースキーが押されたらフェードアウトを開始
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

	// --- 全てのTransformのワールド行列を更新 ---
	TransformUpdater::WorldTransformUpdate(worldTransformTitle_);
	TransformUpdater::WorldTransformUpdate(worldTransformPlayer_);

	// 天球の更新
	skydome_->Update();

	// カメラの行列を更新
	camera_.UpdateMatrix();

	objectColorTitle_.SetColor(colorTitle_);
}

// 描画処理
void TitleScene::Draw() {
	// KamataEngineの描画コマンドを使うためにDirectXCommonを取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	// 天球の描画
	skydome_->Draw();

	// 描画後処理
	KamataEngine::Model::PostDraw();

	// 描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	// --- モデルごとに、対応するTransformを使って描画 ---
	modelTitle_->Draw(worldTransformTitle_, camera_,&objectColorTitle_);
	modelPlayer_->Draw(worldTransformPlayer_, camera_);

	// 描画後処理
	KamataEngine::Model::PostDraw();

	// フェードの描画 (必ず一番最後に描画する)
	fade_->Draw();
}
