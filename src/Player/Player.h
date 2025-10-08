#pragma once
#include "KamataEngine.h"
#include <memory>

class Player {
public:
	// 初期化
	void Initialize(); // [cite: 5]

	// 更新
	void Update(); // [cite: 5]

	// 描画
	void Draw(KamataEngine::Camera* camera); // [cite: 5]

private:
	// Modelの所有権をPlayerが持つためunique_ptrを使用
	std::unique_ptr<KamataEngine::Model> model_;

	KamataEngine::WorldTransform worldTransform_;
};
