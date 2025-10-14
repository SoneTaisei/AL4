#pragma once
#include "KamataEngine.h"

// 前方宣言
class Player;

struct Rect {
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

class CameraController {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(KamataEngine::Camera* camera);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	void SetTarget(Player* target) { target_ = target; }

	void Reset();

	// 追従座標とカメラの座標の差
	KamataEngine::Vector3 targetOffset = {0, 0, -30.0f};

	// カメラの目標座標
	KamataEngine::Vector3 targetPosition_;

	// 座標補間割合
	static inline const float kInterpolationRate = 0.1f;

	// 速度加算率
	static inline const float kVelocityBias = 20.0f;

	// 追従対象の各法へのカメラ移動範囲
	static inline const Rect kCameraTrackingMargin = {-20.0f, 20.0f, -10.0f, 10.0f};

	Rect movableArea_ = {0, 100, 0, 100};

	void SetMovableArea(Rect area) { movableArea_ = area; }

	/// <summary>
	/// 目標角度Zを設定する
	/// </summary>
	void SetTargetAngleZ(float angle) { targetAngleZ_ = angle; }

private:
	// カメラ
	KamataEngine::Camera* camera_;

	Player* target_ = nullptr;

	// カメラの目標角度Z
	float targetAngleZ_ = 0.0f;
};
