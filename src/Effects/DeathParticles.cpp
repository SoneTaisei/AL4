#include "DeathParticles.h"
#include "Utils/TransformUpdater.h" // WorldTransformUpdate を使うために必要
#include <algorithm>

using namespace KamataEngine;

void DeathParticles::Initialize(Model* model, uint32_t textureHandle, Camera* camera, const Vector3& position) {
#ifdef _DEBUG
	// NULLポインタチェック
	assert(model);
	assert(camera);
#endif // _DEBUG

	// 引数で受け取ったデータをメンバ変数に記録する
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	// ワールド変換の初期化
	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position;
		worldTransform.scale_ = {0.3f, 0.3f, 0.3f};
		// 初期スケールを定数から設定
		worldTransform.scale_ = {kInitialScale, kInitialScale, kInitialScale};
	}

	// 色変更オブジェクトの初期化
	objectColor_.Initialize();
	// 色の初期値を設定 (RGBA = {1, 1, 1, 1}) 不透明な白
	color_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// 最初は終了状態にしておき、描画・更新されないようにする
	isFinished_ = true;
}

// Start関数の実装
void DeathParticles::Start(const Vector3& position) {
	// 各パーティクルの座標を開始位置にリセット
	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.translation_ = position;
		// スケールも初期値に戻す
		worldTransform.scale_ = {kInitialScale, kInitialScale, kInitialScale};
	}
	// タイマーとフラグをリセットして再生開始
	counter_ = 0.0f;
	isFinished_ = false;
}

void DeathParticles::Update() {
	// 終了なら何もしない
	if (isFinished_) {
		return;
	}

	// カウンターを1フレーム分の秒数進める(60fps固定を想定)
	counter_ += 1.0f / 60.0f;

	// 存続時間の上限に達したら
	if (counter_ >= kDuration) {
		// 終了扱いにすする
		isFinished_ = true;
	}

	// 1. 経過時間の割合を計算 (0.0 ～ 1.0)
	float timeRatio = counter_ / kDuration;

	// 2. 現在のスケールを計算 (開始時: kInitialScale -> 終了時: 0.0)
	float currentScale = kInitialScale * (1.0f - timeRatio);
	// スケールが負の値にならないように0でクランプ
	currentScale = std::fmaxf(currentScale, 0.0f);

	// 3. 8個のパーティクルにスケールと移動を適用する
	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// 基本となる速度ベクトル（右方向）
		Vector3 velocity = {kSpeed, 0.0f, 0.0f};
		// 回転角を計算する
		float angle = kAngleUnit * i;
		// Z軸まわり回転行列を生成
		Matrix4x4 matrixRotation = TransformUpdater::MakeRoteZMatrix(angle);
		// 基本ベクトルを回転させて、このパーティクルの速度ベクトルを得る
		velocity = TransformUpdater::Transform(velocity, matrixRotation);
		// 座標に速度を加算して移動させる
		worldTransforms_[i].translation_ += velocity;

		// 計算したスケールを適用する
		worldTransforms_[i].scale_ = {currentScale, currentScale, currentScale};
	}

	// 全てのパーティクルのワールド行列を更新する
	// (座標の変更を行列に反映させる)
	for (WorldTransform& worldTransform : worldTransforms_) {
		TransformUpdater::WorldTransformUpdate(worldTransform);
	}
}

void DeathParticles::Draw() {
	// 終了なら何もしない
	if (isFinished_) {
		return;
	}

	// DirectXCommonの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	Model::PreDraw(dxCommon->GetCommandList());

	// モデルの描画
	// (ヒント：範囲for文)
	for (WorldTransform& worldTransform : worldTransforms_) {
		// 第3引数に色変更オブジェクトのアドレスを渡して、計算した色で描画する
		model_->Draw(worldTransform, *camera_, textureHandle_, &objectColor_);
	}

	// 3Dモデル描画後処理
	Model::PostDraw();
}
