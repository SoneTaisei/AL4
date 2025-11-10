#include "Objects/Player.h"
#include "HUD.h"

using namespace KamataEngine;

void HUD::Initialize() {
	hpHandle_ = TextureManager::GetInstance()->Load("HUD/HP.png");
	for (int i = 0; i < 3; ++i) {
		hpSprite_[i] = Sprite::Create(hpHandle_, {50, 50});
		hpSprite_[i]->SetSize({50, 50});
	}
}

void HUD::Update() {}

void HUD::Draw(const Player* player) {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	for (int i = 0; i < player->GetHp(); ++i) {
		Sprite::PreDraw(dxCommon->GetCommandList());
		hpSpritePosition_[i] = {50.0f + float(i) * 60.0f, 50.0f};
		hpSprite_[i]->SetPosition({hpSpritePosition_[i]});
		hpSprite_[i]->Draw();
		Sprite::PostDraw();
	}
}
