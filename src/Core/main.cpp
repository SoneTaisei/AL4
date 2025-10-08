#include "KamataEngine.h"
#include <Windows.h>
#include "Scene/GameScene.h"

using namespace KamataEngine;

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

	std::unique_ptr<GameScene> gameScene;
	gameScene->Initialize();

	/*********************************************************
	 *メインループ
	*********************************************************/

	while (true) {
		// エンジンの更新
		if (Update()) {
			break;
		}

		imguiManager->Begin();

		// 2. 現在シーンの更新
		gameScene->Update();

		imguiManager->End();

		// 描画開始
		dxCommon->PreDraw();

		// 3. 現在シーンの描画
		gameScene->Draw();

		imguiManager->Draw();
		dxCommon->PostDraw();
	}

	/*********************************************************
	*解放処理
	*********************************************************/

	return 0;
}
