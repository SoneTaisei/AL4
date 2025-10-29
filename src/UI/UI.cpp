#include "UI.h"
#include "Objects/Player.h"

using namespace KamataEngine;

void UI::Initialize() {
	hpHandle_ = TextureManager::GetInstance()->Load("UI/HP.png");
	for (int i = 0; i < 3; ++i) {
		hpSprite_[i] = Sprite::Create(hpHandle_, {50, 50});
		hpSprite_[i]->SetSize({50, 50});
	}
}

void UI::Update() {}

void UI::Draw(const Player* player) {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	for (int i = 0; i < player->GetHp(); ++i) {
		Sprite::PreDraw(dxCommon->GetCommandList());
		hpSpritePosition_[i] = {50.0f + float(i) * 60.0f, 50.0f};
		hpSprite_[i]->SetPosition({hpSpritePosition_[i]});
		hpSprite_[i]->Draw();
		Sprite::PostDraw();
	}
}
