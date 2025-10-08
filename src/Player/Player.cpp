#include "Player.h"

using namespace KamataEngine;

void Player::Initialize() { 
	// モデルを作成
	model_ = std::unique_ptr<Model>(Model::CreateSphere());
	// ワールドトランスフォームを初期化
	worldTransform_.Initialize();
	// 2. 定数バッファを生成（これが抜けていた！）
	worldTransform_.CreateConstBuffer();
	// 3. 定数バッファをマッピング（これも抜けていた！）
	worldTransform_.Map();
}

void Player::Update() { 

}

void Player::Draw(KamataEngine::Camera* camera) { 
	camera;
	// // DirectXCommonの取得
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();
	// 3Dモデル描画前処理
	KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

	//model_->Draw(worldTransform_, *camera); 

	// 3Dモデル描画後処理
	KamataEngine::Model::PostDraw();
}