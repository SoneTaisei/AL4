#pragma once

// シーンの識別子
enum class Scene {
	kTitle,
	kStageSelect,
	kGame,
};

/// <summary>
/// 全てのシーンの基底クラス（設計図）
/// </summary>
class BaseScene {
protected:
	// 次のシーンの予約
	Scene nextScene_ = Scene::kTitle;
	// シーンが終了したか
	bool isFinished_ = false;

public:
	// 仮想デストラクタ（おまじない）
	virtual ~BaseScene() = default;

	// 純粋仮想関数（中身は派生クラスで必ず書く）
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	// 終了したかを返す
	bool IsFinished() const { return isFinished_; }
	// 次のシーンを返す
	Scene GetNextScene() const { return nextScene_; }
};