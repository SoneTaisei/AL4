#include "StageSelectScene.h"
#include "Effects/Fade.h"
#include "Effects/Skydome.h"        // Skydomeクラスを使うために必要
#include "Utils/TransformUpdater.h" // WorldTransformの更新に必要
#include <memory>
#include "System/Gamepad.h"

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

	// ESC スプライト用テクスチャをロード（キーボード用）
	escHandle_ = TextureManager::GetInstance()->Load("HUD/esc.png");
	escSprite_ = Sprite::Create(escHandle_, {64, 128});
	if (escSprite_) {
		escSprite_->SetSize({144, 64});
	}

	// コントローラ用 select スプライトをロード
	selectHandle_ = TextureManager::GetInstance()->Load("HUD/select.png");
	selectSprite_ = Sprite::Create(selectHandle_, {64, 128});
	if (selectSprite_) {
		selectSprite_->SetSize({64, 64});
	}

	// --- 3Dオブジェクトの生成 ---
	// 背景天球
	skydome_ = new Skydome();
	// 2. 正しい引数（モデル、テクスチャ、カメラ）を渡す
	skydome_->Initialize(skydomeModel_, skydomeTexture, &camera_);

	// ステージ選択肢の立方体
	for (int i = 0; i < maxStages_; ++i) {
		// int row = i / 5; // 0～4なら0, 5～9なら1
		// int col = i % 5; // 0～4の繰り返し

		// 上の段はY=5.0f, 下の段はY=-5.0fに配置
		Vector3 iconPos = {-10.0f + i * 10.0f, 0.0f, 0.0f};

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

	// 追加: 左スティックの前フレーム状態をリセット
	leftStickPrevPos_ = false;
	leftStickPrevNeg_ = false;

	// 初期: コントローラ非アクティブ（キーボード表示）
	gpActive_ = false;
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

	case Phase::kMain: {
		// --- 入力ソースの「押された」検出（立ち上がりのみ） ---
		Gamepad* gp = Gamepad::GetInstance();
		bool gpTriggered = false;
		if (gp) {
			gpTriggered =
			    gp->IsTriggered(XINPUT_GAMEPAD_A) || gp->IsTriggered(XINPUT_GAMEPAD_B) || gp->IsTriggered(XINPUT_GAMEPAD_X) ||
			    gp->IsTriggered(XINPUT_GAMEPAD_Y) || gp->IsTriggered(XINPUT_GAMEPAD_BACK) || gp->IsTriggered(XINPUT_GAMEPAD_START) ||
			    gp->IsTriggered(XINPUT_GAMEPAD_DPAD_UP) || gp->IsTriggered(XINPUT_GAMEPAD_DPAD_DOWN) ||
			    gp->IsTriggered(XINPUT_GAMEPAD_DPAD_LEFT) || gp->IsTriggered(XINPUT_GAMEPAD_DPAD_RIGHT) ||
			    gp->IsLeftThumbRightTriggered(kStickThreshold) || gp->IsLeftThumbLeftTriggered(kStickThreshold) ||
			    gp->IsLeftThumbUpTriggered(kStickThreshold) || gp->IsLeftThumbDownTriggered(kStickThreshold);
		}

		bool kbTriggered = false;
		auto in = Input::GetInstance();
		if (in) {
			const int keysToCheck[] = {DIK_LEFT, DIK_RIGHT, DIK_UP, DIK_DOWN, DIK_SPACE, DIK_J, DIK_R, DIK_ESCAPE, DIK_RETURN, DIK_A, DIK_D, DIK_W, DIK_S};
			for (int k : keysToCheck) {
				if (in->TriggerKey(char(k))) {
					kbTriggered = true;
					break;
				}
			}
		}

		// 入力が発生した方の表示に切り替える（押された後は持続）
		if (gpTriggered) {
			gpActive_ = true;
		} else if (kbTriggered) {
			gpActive_ = false;
		}

		// 左スティックの正規化値（移動のための立ち上がり検出）
		float leftX = gp ? gp->GetLeftThumbXf() : 0.0f;
		bool stickRightNow = leftX > kStickThreshold;
		bool stickLeftNow = leftX < -kStickThreshold;

		// 入力判定（キーボード・DPadのトリガー、またはスティック立ち上がり）
		bool rightTriggered =
		    Input::GetInstance()->TriggerKey(DIK_D) ||
		    (gp && gp->IsTriggered(XINPUT_GAMEPAD_DPAD_RIGHT)) ||
		    (gp && (gp->IsLeftThumbRightTriggered(kStickThreshold)));
		bool leftTriggered =
		    Input::GetInstance()->TriggerKey(DIK_A) ||
		    (gp && gp->IsTriggered(XINPUT_GAMEPAD_DPAD_LEFT)) ||
		    (gp && (gp->IsLeftThumbLeftTriggered(kStickThreshold)));

		// 右移動
		if (rightTriggered) {
			cursorCol_++;
			if (cursorCol_ > 2) {
				cursorCol_ = 0;
			}
		}
		// 左移動
		if (leftTriggered) {
			cursorCol_--;
			if (cursorCol_ < 0) {
				cursorCol_ = 2;
			}
		}

		// 行と列から最終的なステージ番号を計算
		selectedStageIndex_ = cursorRow_ * 5 + cursorCol_;

		rotationSpeed.y = 0.05f;

		// SPACEキー or A ボタン が押されたら選択
		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || (gp && gp->IsTriggered(XINPUT_GAMEPAD_A))) {
			selectionTimer_ = 0;         // タイマーリセット
			phase_ = Phase::kSelected;   // フェーズ移行
			selectionJumpHeight_ = 0.0f; // 位置はリセット
			jumpVelocity_ = 0.8f;
		}

		// ESCキー or BACK(Select) or START でタイトルへ戻る
		if (Input::GetInstance()->TriggerKey(DIK_ESCAPE) || (gp && (gp->IsTriggered(XINPUT_GAMEPAD_BACK) || gp->IsTriggered(XINPUT_GAMEPAD_START)))) {
			returnToTitle_ = true;
			finished_ = true;
		}

		// 更新: 次フレーム用にスティックの前フレーム状態を保存（Gamepadが内部で管理）
		// leftStickPrevPos_/leftStickPrevNeg_はもう内部判定に依存していないため維持のみ
		leftStickPrevPos_ = stickRightNow;
		leftStickPrevNeg_ = stickLeftNow;
		break;
	}

	case Phase::kSelected:
		selectionTimer_++;

		// --- 演出処理 ---

		// 1. 回転（既存）
		rotationSpeed.y = 0.5f;

		if (selectionJumpHeight_ > -5.0f || jumpVelocity_ > -5.0f) {

			// 重力をかける
			jumpVelocity_ -= 0.05f;
			// 速度分移動する
			selectionJumpHeight_ += jumpVelocity_;

			// ★ここが重要：着地判定（貫通防止）
			// もし地面（0.0f）より下に行ってしまったら
			if (selectionJumpHeight_ <= -5.0f) {
				selectionJumpHeight_ = -5.0f; // 位置を強制的に地面(0)に戻す
				jumpVelocity_ = -5.0f;        // 速度を0にして止める
			}
		}

		// ※解説：
		// 最初は velocity が 0.8 なので上昇します。
		// やがて velocity がマイナスになると落下し始めます。
		// selectionJumpHeight_ が -4.0f に達すると、ちょうど箱の中（高さ0）になります。

		// 3. キューブのぼよん演出（既存のまま）
		if (selectionTimer_ >= 30) {
			float wobbleSpeed = 0.5f;
			float wobbleAmount = 0.5f;
			float wobble = std::sin((float)selectionTimer_ * wobbleSpeed) * wobbleAmount;
			float baseScale = 2.0f;
			stageCubeTransforms_[selectedStageIndex_]->scale_ = {baseScale - wobble, baseScale + wobble, baseScale - wobble};
		}

		if (jumpVelocity_ < 0.0f) {
			// 0.95倍ずつ小さくする（最小0まで）
			cursorTransform_->scale_.x *= 0.95f;
			cursorTransform_->scale_.y *= 0.95f;
			cursorTransform_->scale_.z *= 0.95f;
		}

		// 指定時間でフェードアウト
		if (selectionTimer_ >= kSelectionDuration) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
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
	cursorTransform_->translation_ = {selectedCubePos.x, selectedCubePos.y + 4.0f + selectionJumpHeight_, selectedCubePos.z};

	// カーソルをクルクル回転させる演出 (変更なし)
	cursorTransform_->rotation_.y += rotationSpeed.y;

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

	// ESC / SELECT スプライトの暗転判定（キーボードは esc.png、コントローラは select.png）
	Sprite::PreDraw(dxCommon->GetCommandList());
	{
		if (gpActive_ && selectSprite_) {
			// コントローラ用 select 表示 (暗転は START/BACK 押下で反映)
			Gamepad* gp = Gamepad::GetInstance();
			if (gp && (gp->IsPressed(XINPUT_GAMEPAD_BACK) || gp->IsPressed(XINPUT_GAMEPAD_START))) {
				selectSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
			} else {
				selectSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
			}
			selectSprite_->SetPosition({64,128});
			selectSprite_->Draw();
		} else if (escSprite_) {
			// キーボード用 esc 表示（既存）
			if (Input::GetInstance()->PushKey(DIK_ESCAPE)) {
				escSprite_->SetColor({0.5f, 0.5f, 0.5f, 1.0f});
			} else {
				escSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
			}
			escSprite_->SetPosition({64,128});
			escSprite_->Draw();
		}
	}
	Sprite::PostDraw();
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

	// Sprite オブジェクトはエンジン側で管理される実装の可能性が高いため明示削除は行わない（GameScene の実装に合わせる）
}