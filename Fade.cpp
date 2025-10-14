#include "Fade.h"
#include <algorithm>

using namespace KamataEngine;

// 初期化処理
void Fade::Initialize() {
	// テクスチャハンドル
	textureHandle_ = TextureManager::Load("white1x1.png");

	// フェード用の黒いスプライトを生成する
	sprite_ = Sprite::Create(textureHandle_, {});

	// 画面全体に表示されるようにサイズを設定
	// ※ KamataEngineの仕様に合わせて画面サイズを取得してください。
	// ここでは仮に1280x720とします。
	sprite_->SetSize(Vector2{1280.0f, 720.0f});

	// 色を黒（不透明）に設定
	sprite_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
}

// 更新処理 (今回は特に処理なし)
void Fade::Update() {
	// 状態によって処理を分岐
	switch (status_) {
	case Status::None:
		// 何もしない
		break;

	case Status::FadeIn: { // フェードイン中の更新処理
		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;

		// フェードインの進行度（0.0f ～ 1.0f）
		float progress = std::fminf(counter_ / duration_, 1.0f);
		// アルファ値を計算 (1.0 -> 0.0)
		float alpha = 1.0f - progress;
		sprite_->SetColor({0.0f, 0.0f, 0.0f, alpha});
	} break;

	case Status::FadeOut: { // フェードアウト中の更新処理
		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;

		// フェードアウトの進行度（0.0f ～ 1.0f）
		float progress = std::fminf(counter_ / duration_, 1.0f);
		// アルファ値を計算 (0.0 -> 1.0)
		float alpha = progress;
		sprite_->SetColor({0.0f, 0.0f, 0.0f, alpha});
	} break;
	}
}

// 描画処理
void Fade::Draw() {
	// フェード中でなければ何もしない
	if (status_ == Status::None) {
		return;
	}
	// スプライト描画の前処理
	Sprite::PreDraw(KamataEngine::DirectXCommon::GetInstance()->GetCommandList());

	// スプライトを描画
	sprite_->Draw();

	// スプライト描画の後処理
	Sprite::PostDraw();
}

// フェード開始関数の実装
void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;

	// フェードイン開始時はアルファ値を1(不透明)に、
	// フェードアウト開始時はアルファ値を0(透明)に初期化する
	if (status_ == Status::FadeIn) {
		sprite_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	} else if (status_ == Status::FadeOut) {
		sprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
	}
}

// フェード終了関数の実装
void Fade::Stop() { status_ = Status::None; }

// ★追加: フェード終了判定関数の実装
bool Fade::IsFinished() const {
	// フェードインかフェードアウト中で、カウンターが持続時間を超えたらtrue
	// 資料のコードをシンプルに実装しています
	if ((status_ == Status::FadeIn || status_ == Status::FadeOut) && counter_ >= duration_) {
		return true;
	}
	return false;
}
