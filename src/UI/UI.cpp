#include "UI.h"
#include "Objects/Player.h"

using namespace KamataEngine;

void UI::Initialize() {
	pausdHandle_ = TextureManager::GetInstance()->Load("UI/pause.png");
	backGroundHnadle_ = TextureManager::GetInstance()->Load("white1x1.png");
	arrowHnadle_ = TextureManager::GetInstance()->Load("UI/arrow.png");
	pausdSprite_ = Sprite::Create(pausdHandle_, {});
	pausdSprite_->SetSize({768, 432});
	backGroundSprite_ = Sprite::Create(backGroundHnadle_, {});
	backGroundSprite_->SetSize({1280, 720});
	arrowSprite_ = Sprite::Create(arrowHnadle_, {});
	arrowSprite_->SetSize({80, 32});
}

void UI::Update() {}

void UI::Draw(int pauseMenuIndex) {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	Sprite::PreDraw(dxCommon->GetCommandList());
	backGroundSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.8f});
	backGroundSprite_->Draw();
	pausdSpritePosition_ = {1280 / 2 - pausdSprite_->GetSize().x / 2.0f, 720 / 2 - pausdSprite_->GetSize().y / 2.0f};
	pausdSprite_->SetPosition({pausdSpritePosition_});
	pausdSprite_->Draw();
	if (pauseMenuIndex == 0) {
		arrowSprite_->SetPosition({350, 370});
	} else if (pauseMenuIndex == 1) {
		arrowSprite_->SetPosition({350, 470});
	}
	arrowSprite_->Draw();
	Sprite::PostDraw();
}
