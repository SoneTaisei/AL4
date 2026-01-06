#include "Gamepad.h"

Gamepad::Gamepad() {
	prevButtons_ = 0;
	currButtons_ = 0;
	prevLeftThumbX_ = 0;
	prevLeftThumbY_ = 0;
	leftThumbX_ = 0;
	leftThumbY_ = 0;
}

Gamepad* Gamepad::GetInstance() {
	static Gamepad instance;
	return &instance;
}

void Gamepad::Update() {
	// 前フレームの状態を保存
	prevButtons_ = currButtons_;
	prevLeftThumbX_ = leftThumbX_;
	prevLeftThumbY_ = leftThumbY_;

	currButtons_ = 0;
	leftThumbX_ = 0;
	leftThumbY_ = 0;

	// プレイヤー0 の状態を取得
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(state));
	DWORD dwResult = XInputGetState(0, &state);
	if (dwResult == ERROR_SUCCESS) {
		currButtons_ = state.Gamepad.wButtons;
		leftThumbX_ = state.Gamepad.sThumbLX;
		leftThumbY_ = state.Gamepad.sThumbLY;
	} else {
		// コントローラ未接続時は0
		currButtons_ = 0;
		leftThumbX_ = 0;
		leftThumbY_ = 0;
	}
}

static float NormalizeThumb(SHORT v) {
	constexpr float normPos = 32767.0f;
	constexpr float normNeg = 32768.0f;
	return (v >= 0) ? (v / normPos) : (v / normNeg);
}

float Gamepad::GetLeftThumbXf() const { return NormalizeThumb(leftThumbX_); }
float Gamepad::GetLeftThumbYf() const { return NormalizeThumb(leftThumbY_); }
float Gamepad::GetPrevLeftThumbXf() const { return NormalizeThumb(prevLeftThumbX_); }
float Gamepad::GetPrevLeftThumbYf() const { return NormalizeThumb(prevLeftThumbY_); }

bool Gamepad::IsLeftThumbRightTriggered(float threshold) const {
	return GetPrevLeftThumbXf() <= threshold && GetLeftThumbXf() > threshold;
}
bool Gamepad::IsLeftThumbLeftTriggered(float threshold) const {
	return GetPrevLeftThumbXf() >= -threshold && GetLeftThumbXf() < -threshold;
}
bool Gamepad::IsLeftThumbUpTriggered(float threshold) const {
	return GetPrevLeftThumbYf() <= threshold && GetLeftThumbYf() > threshold;
}
bool Gamepad::IsLeftThumbDownTriggered(float threshold) const {
	return GetPrevLeftThumbYf() >= -threshold && GetLeftThumbYf() < -threshold;
}