#include "CameraController.h"
#include "Easing.h"
#include "Player.h"
#include <algorithm>
#include <numbers>

using namespace KamataEngine;

void CameraController::Initialize(KamataEngine::Camera* camera) { camera_ = camera; }

void CameraController::Update() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	const Vector3& targetVelocity = target_->GetVelocity();

	// 追従対象とオフセットからカメラの座標を計算
	targetPosition_ = {
	    targetWorldTransform.translation_.x + targetOffset.x + targetVelocity.x * kVelocityBias, targetWorldTransform.translation_.y + targetOffset.y + targetVelocity.y * kVelocityBias,
	    targetWorldTransform.translation_.z + targetOffset.z + targetVelocity.z * kVelocityBias};

	camera_->translation_.x = Lerp(camera_->translation_.x, targetPosition_.x, kInterpolationRate);
	camera_->translation_.y = Lerp(camera_->translation_.y, targetPosition_.y, kInterpolationRate);
	camera_->translation_.z = Lerp(camera_->translation_.z, targetPosition_.z, kInterpolationRate);

	// 現在のカメラのX座標が、追従対象のX座標からマージンを引いた値より小さければ、その値に補正
	camera_->translation_.x = (std::max)(camera_->translation_.x, targetWorldTransform.translation_.x + kCameraTrackingMargin.left);
	// 現在のカメラのX座標が、追従対象のX座標にマージンを足した値より大きければ、その値に補正
	camera_->translation_.x = (std::min)(camera_->translation_.x, targetWorldTransform.translation_.x + kCameraTrackingMargin.right);
	// 現在のカメラのY座標が、追従対象のY座標からマージンを引いた値より小さければ、その値に補正
	camera_->translation_.y = (std::max)(camera_->translation_.y, targetWorldTransform.translation_.y + kCameraTrackingMargin.bottom);
	// 現在のカメラのY座標が、追従対象のY座標にマージンを足した値より大きければ、その値に補正
	camera_->translation_.y = (std::min)(camera_->translation_.y, targetWorldTransform.translation_.y + kCameraTrackingMargin.top);

	// カメラの移動範囲を制限
	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);

	// --- カメラの回転処理 ---
	// 目標角度に向かって滑らかに補間する
	float currentAngle = camera_->rotation_.z;
	float diff = std::fmodf(targetAngleZ_ - currentAngle + std::numbers::pi_v<float>, 2.0f * std::numbers::pi_v<float>) - std::numbers::pi_v<float>;
	float lerpedAngle = currentAngle + diff * 0.2f; // 0.2f の部分で回転の速さを調整

	// 計算結果をカメラの回転に反映
	camera_->rotation_.z = lerpedAngle;

	camera_->UpdateMatrix();
}

void CameraController::Reset() {
	// 追従座標のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	camera_->translation_ = {targetWorldTransform.translation_.x + targetOffset.x, targetWorldTransform.translation_.y + targetOffset.y, targetWorldTransform.translation_.z + targetOffset.z};
}
