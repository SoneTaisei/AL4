#pragma once
#include "KamataEngine.h"
#include <memory>

class Player;

// ゲームシーン
class GameScene { //
public:
	// 初期化
	void Initialize(); // [cite: 5]

	// 更新
	void Update(); // [cite: 5]

	// 描画
	void Draw(); // [cite: 5]

	// デストラクタの宣言を追加
	~GameScene();

private:
	// カメラ
	std::unique_ptr<KamataEngine::Camera> camera_;

	std::unique_ptr<Player> player_;
};