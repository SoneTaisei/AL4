#include "UI.h"
#include "Objects/Player.h"
#include "System/Gamepad.h"
#include <algorithm>

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

	// コントローラ用スプライトをロード
	aHandle_ = TextureManager::GetInstance()->Load("HUD/A.png");
	xHandle_ = TextureManager::GetInstance()->Load("HUD/X.png");
	selectHandle_ = TextureManager::GetInstance()->Load("HUD/select.png");

	if (aHandle_) aSprite_ = Sprite::Create(aHandle_, {});
	if (xHandle_) xSprite_ = Sprite::Create(xHandle_, {});
	if (selectHandle_) selectSprite_ = Sprite::Create(selectHandle_, {});

	if (aSprite_) aSprite_->SetSize({64, 64});
	if (xSprite_) xSprite_->SetSize({64, 64});
	if (selectSprite_) selectSprite_->SetSize({144, 64});

	// 初期はキーボード表示
	lastInputIsGamepad_ = false;
}

void UI::Update() {
	// 入力ソース判定:
	// - ゲームパッド入力があれば gamepad と判定
	// - 逆にキーボード入力があれば keyboard と判定
	// 優先は「最後に操作された方」

	// 1) ゲームパッドの活動検出
	Gamepad* gp = Gamepad::GetInstance();
	bool gpActive = false;
	if (gp) {
		// ボタン（トリガー／押下）またはスティック（閾値越え）で検出
		gpActive = gp->IsPressed(XINPUT_GAMEPAD_A) || gp->IsPressed(XINPUT_GAMEPAD_B) || gp->IsPressed(XINPUT_GAMEPAD_X) || gp->IsPressed(XINPUT_GAMEPAD_Y) ||
		           gp->IsPressed(XINPUT_GAMEPAD_BACK) || gp->IsPressed(XINPUT_GAMEPAD_START) ||
		           gp->IsPressed(XINPUT_GAMEPAD_DPAD_UP) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_DOWN) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) ||
		           std::abs(gp->GetLeftThumbXf()) > kGamepadStickThreshold || std::abs(gp->GetLeftThumbYf()) > kGamepadStickThreshold;
	}

	// 2) キーボードの活動検出（よく使うキーで判定）
	bool kbActive = false;
	auto in = Input::GetInstance();
	if (in) {
		const int keysToCheck[] = {DIK_LEFT, DIK_RIGHT, DIK_UP, DIK_DOWN, DIK_SPACE, DIK_J, DIK_R, DIK_ESCAPE, DIK_RETURN, DIK_A, DIK_D, DIK_W, DIK_S};
		for (int k : keysToCheck) {
			// PushKey の引数が BYTE（1バイト）に落ちるため、明示的にキャストして警告を抑制
			if (in->PushKey(static_cast<unsigned char>(k))) {
				kbActive = true;
				break;
			}
		}
	}

	// 最終入力更新（直近で押された方を優先）
	if (gpActive && !lastInputIsGamepad_) {
		lastInputIsGamepad_ = true;
	} else if (kbActive && lastInputIsGamepad_) {
		lastInputIsGamepad_ = false;
	}
}

void UI::Draw(int pauseMenuIndex) {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	Sprite::PreDraw(dxCommon->GetCommandList());
	backGroundSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.8f});
	backGroundSprite_->Draw();
	pausdSpritePosition_ = {1280 / 2 - pausdSprite_->GetSize().x / 2.0f, 720 / 2 - pausdSprite_->GetSize().y / 2.0f};
	pausdSprite_->SetPosition({pausdSpritePosition_});
	pausdSprite_->Draw();

	if (!lastInputIsGamepad_) {
		// キーボード UI（既存）
		if (pauseMenuIndex == 0) {
			arrowSprite_->SetPosition({350, 370});
		} else if (pauseMenuIndex == 1) {
			arrowSprite_->SetPosition({350, 470});
		}
		arrowSprite_->Draw();
	} else {
		// コントローラ UI: A (決定) と X (戻る/操作) と select (ポーズ) を表示
		// 位置は画面下部中央付近に配置
		float baseY = 600.0f;
		// A は決定 (右側寄せ)
		if (aSprite_) {
			aSprite_->SetPosition({640.0f + 48.0f, baseY});
			aSprite_->Draw();
		}
		// X はキャンセル/操作説明（左寄せ）
		if (xSprite_) {
			xSprite_->SetPosition({640.0f - 112.0f, baseY});
			xSprite_->Draw();
		}
		// select は中央やや下に表示
		if (selectSprite_) {
			selectSprite_->SetPosition({640.0f - selectSprite_->GetSize().x / 2.0f, baseY + 70.0f});
			selectSprite_->Draw();
		}
	}

	Sprite::PostDraw();
}
