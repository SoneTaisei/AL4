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

	// 初期はキーボード表示
	lastInputIsGamepad_ = false;
}

void UI::Update() {
	// UIレベルでの「最後に使われた入力デバイス」を軽く更新します（押しっぱなしではなくトリガーで切り替えたいので
	// 実際の切り替えは HandlePauseInput のトリガー判定で行います）。ここではスティックやキーが押されていると
	// lastInputIsGamepad_ の候補値を更新するだけに留めます。

	Gamepad* gp = Gamepad::GetInstance();
	bool gpActive = false;
	if (gp) {
		gpActive = gp->IsPressed(XINPUT_GAMEPAD_A) || gp->IsPressed(XINPUT_GAMEPAD_B) || gp->IsPressed(XINPUT_GAMEPAD_X) || gp->IsPressed(XINPUT_GAMEPAD_Y) ||
		           gp->IsPressed(XINPUT_GAMEPAD_BACK) || gp->IsPressed(XINPUT_GAMEPAD_START) ||
		           gp->IsPressed(XINPUT_GAMEPAD_DPAD_UP) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_DOWN) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_LEFT) || gp->IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT) ||
		           std::abs(gp->GetLeftThumbXf()) > kGamepadStickThreshold || std::abs(gp->GetLeftThumbYf()) > kGamepadStickThreshold;
	}

	bool kbActive = false;
	auto in = Input::GetInstance();
	if (in) {
		const int keysToCheck[] = {DIK_LEFT, DIK_RIGHT, DIK_UP, DIK_DOWN, DIK_SPACE, DIK_J, DIK_R, DIK_ESCAPE, DIK_RETURN, DIK_A, DIK_D, DIK_W, DIK_S};
		for (int k : keysToCheck) {
			if (in->PushKey(static_cast<unsigned char>(k))) {
				kbActive = true;
				break;
			}
		}
	}

	// 押されている方を lastInputIsGamepad_ の候補にする（実際の切替はトリガーで行う）
	// ここでは候補判定のみ行い、永続切替は HandlePauseInput で行う設計のままにします。
	(void)gpActive;
	(void)kbActive;
}

void UI::HandlePauseInput(int& pauseMenuIndex, bool& outConfirm, bool& outCancel) {
	outConfirm = false;
	outCancel = false;

	Gamepad* gp = Gamepad::GetInstance();
	auto in = Input::GetInstance();

	// --- 1) ゲームパッドのトリガー検出（立ち上がり）---
	bool gpTriggeredUp = false;
	bool gpTriggeredDown = false;
	bool gpTriggeredConfirm = false;
	bool gpTriggeredCancel = false;

	if (gp) {
		// DPad
		if (gp->IsTriggered(XINPUT_GAMEPAD_DPAD_UP)) gpTriggeredUp = true;
		if (gp->IsTriggered(XINPUT_GAMEPAD_DPAD_DOWN)) gpTriggeredDown = true;
		// A = confirm, B or BACK = cancel
		if (gp->IsTriggered(XINPUT_GAMEPAD_A)) gpTriggeredConfirm = true;
		if (gp->IsTriggered(XINPUT_GAMEPAD_B) || gp->IsTriggered(XINPUT_GAMEPAD_BACK)) gpTriggeredCancel = true;
		// スティックの立ち上がり
		if (gp->IsLeftThumbUpTriggered(kGamepadStickThreshold)) gpTriggeredUp = true;
		if (gp->IsLeftThumbDownTriggered(kGamepadStickThreshold)) gpTriggeredDown = true;
	}

	// --- 2) キーボードのトリガー検出 ---
	bool kbTriggeredUp = false;
	bool kbTriggeredDown = false;
	bool kbTriggeredConfirm = false;
	bool kbTriggeredCancel = false;

	if (in) {
		if (in->TriggerKey(DIK_W) || in->TriggerKey(DIK_UP)) kbTriggeredUp = true;
		if (in->TriggerKey(DIK_S) || in->TriggerKey(DIK_DOWN)) kbTriggeredDown = true;
		if (in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_RETURN)) kbTriggeredConfirm = true;
		if (in->TriggerKey(DIK_ESCAPE)) kbTriggeredCancel = true;
	}

	// --- 3) トリガーが発生したら入力デバイスを永続切替（押された後も切り替えたまま） ---
	if (gpTriggeredUp || gpTriggeredDown || gpTriggeredConfirm || gpTriggeredCancel) {
		lastInputIsGamepad_ = true;
	}
	if (kbTriggeredUp || kbTriggeredDown || kbTriggeredConfirm || kbTriggeredCancel) {
		lastInputIsGamepad_ = false;
	}

	// --- 4) 実際のメニュー移動・決定・キャンセル処理 ---
	// 上
	if (gpTriggeredUp || kbTriggeredUp) {
		pauseMenuIndex = (pauseMenuIndex + UI::kPauseMenuCount - 1) % UI::kPauseMenuCount;
	}
	// 下
	if (gpTriggeredDown || kbTriggeredDown) {
		pauseMenuIndex = (pauseMenuIndex + 1) % UI::kPauseMenuCount;
	}
	// 決定
	if (gpTriggeredConfirm || kbTriggeredConfirm) {
		outConfirm = true;
	}
	// キャンセル（ESC 相当）
	if (gpTriggeredCancel || kbTriggeredCancel) {
		outCancel = true;
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

	// 矢印は選択インデックスに合わせて表示
	if (pauseMenuIndex == 0) {
		arrowSprite_->SetPosition({350, 370});
	} else if (pauseMenuIndex == 1) {
		arrowSprite_->SetPosition({350, 470});
	}
	arrowSprite_->Draw();

	Sprite::PostDraw();
}
