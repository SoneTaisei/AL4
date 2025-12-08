#include "KamataEngine.h"
#include "Scenes/GameScene.h"
#include "Scenes/StageSelectScene.h"
#include "Scenes/TitleScene.h"
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


void ForceChangeScene(Scene next) {
	// 現在のシーンインスタンスをすべて安全に解放
	delete titleScene;
	titleScene = nullptr;
	delete stageSelectScene;
	stageSelectScene = nullptr;
	delete gameScene;
	gameScene = nullptr;

	// グローバルのシーン変数を更新
	scene = next;

	// 新しいシーンを初期化
	switch (scene) {
	case Scene::kTitle:
		titleScene = new TitleScene();
		titleScene->Initialize();
		break;
	case Scene::kStageSelect:
		stageSelectScene = new StageSelectScene();
		stageSelectScene->Initialize();
		break;
	case Scene::kGame:
		gameScene = new GameScene();
		// GameSceneの初期化にはステージ番号が必須なため、
		// デバッグ用にステージ1を仮で指定します。
		gameScene->Initialize(1);
		break;
	}
}

// シーン切り替え処理
void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene && titleScene->GetIsFinished()) {
			delete titleScene;
			titleScene = nullptr;

			// 次のシーンをステージセレクトに変更
			stageSelectScene = new StageSelectScene();
			stageSelectScene->Initialize();
			scene = Scene::kStageSelect;
		}
		break;

	case Scene::kStageSelect: // ★ステージセレクトのケース
		if (stageSelectScene && stageSelectScene->GetIsFinished()) {
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
		if (gameScene && gameScene->GetIsFinished()) {
			delete gameScene;
			gameScene = nullptr;

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
	case Scene::kStageSelect: 
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
	case Scene::kStageSelect:
		stageSelectScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	/*********************************************************
	 *初期化処理
	 *********************************************************/
	Initialize(L"LE2B_13_ソネ_タイセイ_オバケパニック");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	/*********************************************************
	 *シーンの初期化
	 *********************************************************/
	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize();

	// 初期シーンがkTitle (enum値=1) なので、インデックスは0に設定します。
	static int selectedSceneIndex = 0;

	/*********************************************************
	 *メインループ
	 *********************************************************/
	while (true) {
		// エンジンの更新
		if (Update()) {
			break;
		}

		imguiManager->Begin();

#ifdef _DEBUG
		/*********************************************************
		 *ImGuiデバッグ用UI
		 *********************************************************/
		ImGui::Begin("Scene Controller");

		const char* sceneItems[] = {"Title", "Stage Select", "Game"};

		// ImGuiのコンボボックスに永続的な変数(static変数)を渡す。
		// これにより、ユーザーの選択が正しく保持されるようになります。
		ImGui::Combo("Scene", &selectedSceneIndex, sceneItems, IM_ARRAYSIZE(sceneItems));

		// 「Change Scene」ボタンが押されたらシーンを強制的に切り替える
		if (ImGui::Button("Change Scene")) {
			// 選択されているインデックスから、切り替えたいシーンのenum値を取得
			Scene nextScene = static_cast<Scene>(selectedSceneIndex + 1);

			// 現在のシーンと選択されたシーンが異なる場合のみ、切り替え処理を実行
			if (scene != nextScene) {
				ForceChangeScene(nextScene);
			}
		}
		ImGui::End();
#endif // _DEBUG

		// 1. シーン切り替え (ゲームプレイによる自然な遷移)
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
	delete titleScene;
	delete stageSelectScene;
	delete gameScene;

	// エンジンを終了させる処理
	Finalize();

	return 0;
}
