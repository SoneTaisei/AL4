#include "GameScene.h"
#include "KamataEngine.h"
#include "StageSelectScene.h"
#include "TitleScene.h"
#include <Windows.h>
using namespace KamataEngine;

// シーンの型
enum class Scene {
	kUnknown, // 不明
	kTitle,
	kStageSelect,
	kGame,
};

// 現在のシーン
Scene scene = Scene::kUnknown;
// 各シーンのインスタンスを保持するポインタ
TitleScene* titleScene = nullptr;
StageSelectScene* stageSelectScene = nullptr;
GameScene* gameScene = nullptr;

// ==================================================
// ★★★ 新しいグローバル関数を追加 ★★★
// ==================================================

// シーン切り替え処理
void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene && titleScene->IsFinished()) {
			delete titleScene;
			titleScene = nullptr;

			// 次のシーンをステージセレクトに変更
			stageSelectScene = new StageSelectScene();
			stageSelectScene->Initialize();
			scene = Scene::kStageSelect;
		}
		break;

	case Scene::kStageSelect: // ★ステージセレクトのケースを追加
		if (stageSelectScene && stageSelectScene->IsFinished()) {
			// 選択されたステージ番号を取得
			int stageNo = stageSelectScene->GetSelectedStageNo();

			delete stageSelectScene;
			stageSelectScene = nullptr;

			// 次のシーン（ゲーム）にステージ番号を渡す
			gameScene = new GameScene();
			gameScene->Initialize(stageNo); // ← ステージ番号を渡す
			scene = Scene::kGame;
		}
		break;

	case Scene::kGame:
		if (gameScene && gameScene->IsFinished()) {
			delete gameScene;
			gameScene = nullptr;

			// 変更前（おそらくこうなっているはず）：
			// titleScene = new TitleScene();
			// titleScene->Initialize();
			// scene = Scene::kTitle;

			// ★★★ 変更後 ★★★
			// ステージセレクトに戻るようにする
			stageSelectScene = new StageSelectScene();
			stageSelectScene->Initialize();
			scene = Scene::kStageSelect;
		}
		break;
	}
}

// UpdateScene関数
void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kStageSelect: // ★追加
		stageSelectScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	}
}

// DrawScene関数
void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kStageSelect: // ★追加
		stageSelectScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	/*********************************************************
	 *初期化処理
	 *********************************************************/
	Initialize(L"LE2B_12_ソネ_タイセイ_AL3");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	/*********************************************************
	 *シーンの初期化
	 *********************************************************/
	// 最初のシーンをタイトルに設定
	scene = Scene::kTitle;
	// ローカル変数ではなく、グローバルのポインタを使う
	titleScene = new TitleScene();
	titleScene->Initialize();

	/*********************************************************
	 *メインループ
	 *********************************************************/

	while (true) {
		// エンジンの更新
		if (Update()) {
			break;
		}

		imguiManager->Begin();

		// 1. シーン切り替え
		ChangeScene();
		// 2. 現在シーンの更新
		UpdateScene();

		imguiManager->End();

		// 描画開始
		dxCommon->PreDraw();

		// 3. 現在シーンの描画
		DrawScene();

		imguiManager->Draw();
		dxCommon->PostDraw();
	}

	/*********************************************************
	 *解放処理
	 *********************************************************/

	// ゲームを終了したらタイトルシーンを開放
	delete titleScene;
	delete gameScene;

	// エンジンを終了させる処理
	Finalize();

	return 0;
}
