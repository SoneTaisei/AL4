#include "Goal.h"
#include "TransformUpdater.h"

void Goal::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position) {
	model_ = model;
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
}

void Goal::Update() {
	// ゴールが動いたり回転したりする場合はここに書く
	// 今回は静的なので何もしない

	// 行列の更新
	TransformUpdater::WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Goal::Draw(const KamataEngine::Camera& camera) {

	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	if (model_) {
		model_->Draw(worldTransform_, camera);
	}

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();

}

AABB Goal::GetAABB() {
	// ゴールのサイズ（当たり判定の大きさ）
	const float kWidth = 1.0f;
	const float kHeight = 1.0f;

	AABB aabb;
	aabb.min = {worldTransform_.translation_.x - (kWidth / 2.0f), worldTransform_.translation_.y - (kHeight / 2.0f), worldTransform_.translation_.z - (kWidth / 2.0f)};
	aabb.max = {worldTransform_.translation_.x + (kWidth / 2.0f), worldTransform_.translation_.y + (kHeight / 2.0f), worldTransform_.translation_.z + (kWidth / 2.0f)};
	return aabb;
}