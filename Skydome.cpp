#include "Skydome.h"
#include "TransformUpdater.h"

using namespace KamataEngine;

void Skydome::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera) {
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransform_.scale_.x = 1.0f;
	worldTransform_.scale_.y = 1.0f;
	worldTransform_.scale_.z = 1.0f;
	worldTransform_.translation_.x = 1.0f;
	worldTransform_.translation_.y = 1.0f;
}

void Skydome::Update() {

	// 行列の更新と転送
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Skydome::Draw() {
	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	model_->Draw(worldTransform_, *camera_, textureHandle_);

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();
}