#include "GameScene.h"
#include"Player/Player.h"

using namespace KamataEngine; 

// デストラクタの定義を追加 (中身は空で良い)
GameScene::~GameScene() = default;

void GameScene::Initialize() { 
	//// --- Cameraの初期化 ---
	//Camera* camera = new Camera();
	//// ↓↓↓ おそらくコレが抜けていました！ ↓↓↓
	//camera->CreateConstBuffer();
	//// ↑↑↑ 定数バッファを作成する関数を呼び出す ↑↑↑
	//camera->Initialize();
	//camera_.reset(camera);

	//// --- Playerの初期化 ---
	//Player* player = new Player();
	//player->Initialize();
	//player_.reset(player);
}

void GameScene::Update() {
	// ここにインゲームの更新処理を書く [cite: 8]
}

void GameScene::Draw() { 
	// DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	//player_->Draw(camera_.get());

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();
}