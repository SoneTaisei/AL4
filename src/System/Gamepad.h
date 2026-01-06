#pragma once
#include <Windows.h>
#include <Xinput.h>

class Gamepad {
public:
	// シングルトン取得
	static Gamepad* GetInstance();

	// フレーム更新（必ず毎フレーム最初に呼ぶ）
	void Update();

	// ボタン状態
	bool IsPressed(WORD button) const { return (currButtons_ & button) != 0; }
	bool IsTriggered(WORD button) const { return ((currButtons_ & button) != 0) && ((prevButtons_ & button) == 0); }

	// 左スティックの生値（SHORT）
	SHORT GetLeftThumbX() const { return leftThumbX_; }
	SHORT GetLeftThumbY() const { return leftThumbY_; }
	SHORT GetPrevLeftThumbX() const { return prevLeftThumbX_; }
	SHORT GetPrevLeftThumbY() const { return prevLeftThumbY_; }

	// 左スティックを -1.0 .. 1.0 に正規化した値（呼び出し側でデッドゾーン処理）
	float GetLeftThumbXf() const;
	float GetLeftThumbYf() const;
	float GetPrevLeftThumbXf() const;
	float GetPrevLeftThumbYf() const;

	// スティック立ち上がり判定（閾値越え：右/左/上/下）
	bool IsLeftThumbRightTriggered(float threshold) const;
	bool IsLeftThumbLeftTriggered(float threshold) const;
	bool IsLeftThumbUpTriggered(float threshold) const;
	bool IsLeftThumbDownTriggered(float threshold) const;

private:
	Gamepad();
	WORD prevButtons_ = 0;
	WORD currButtons_ = 0;
	SHORT prevLeftThumbX_ = 0;
	SHORT prevLeftThumbY_ = 0;
	SHORT leftThumbX_ = 0;
	SHORT leftThumbY_ = 0;
};