#include "HUD.h"
#include "Objects/Player.h"
#include <string>

using namespace KamataEngine;

void HUD::Initialize() {
	// HPの読み込み（既存）
	hpHandle_ = TextureManager::GetInstance()->Load("HUD/HP.png");
	for (int i = 0; i < 3; ++i) {
		hpSprite_[i] = Sprite::Create(hpHandle_, {50, 50});
		hpSprite_[i]->SetSize({50, 50});

		// 同じ hpHandle_ (ハート画像) を使って別のスプライトを作る
		hpBackSprite_[i] = Sprite::Create(hpHandle_, {50, 50});
		hpBackSprite_[i]->SetSize({50, 50});

		// 元の画像の色にこの色が乗算されるため、(0,0,0)を指定すると真っ黒になります
		hpBackSprite_[i]->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	}

	// ★追加: 「STAGE 1-」画像の読み込み
	// ※画像ファイル "Resources/HUD/stage_text.png" を用意してください
	stageTextHandle_ = TextureManager::GetInstance()->Load("number/STAGE1.png");
	stageTextSprite_ = Sprite::Create(stageTextHandle_, {0, 0});
	// サイズや基準点は適宜調整してください（例: 幅256, 高さ64）
	stageTextSprite_->SetSize({384.0f, 64.0f});

	// ★追加: 数字画像（0.png ～ 9.png）の読み込み
	for (int i = 0; i < 10; ++i) {
		// ファイルパスを作成（例: HUD/0.png, HUD/1.png ...）
		std::string path = "number/" + std::to_string(i) + ".png";
		numberHandles_[i] = TextureManager::GetInstance()->Load(path);

		// スプライト生成
		numberSprites_[i] = Sprite::Create(numberHandles_[i], {0, 0});
		// サイズを適宜設定（例: 64x64）
		numberSprites_[i]->SetSize({64.0f, 64.0f});
	}
}

void HUD::Update() {}

void HUD::Draw(const Player* player) {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 描画開始
	Sprite::PreDraw(dxCommon->GetCommandList());

	for (int i = 0; i < 3; ++i) {
		hpSpritePosition_[i] = {50.0f + float(i) * 60.0f, 50.0f};

		// 1. まず「黒くしたハート」を常に描画（背景）
		hpBackSprite_[i]->SetPosition(hpSpritePosition_[i]);
		hpBackSprite_[i]->Draw();

		// 2. 現在のHPがある場合だけ、上から「元のハート」を描画
		if (i < player->GetHp()) {
			hpSprite_[i]->SetPosition(hpSpritePosition_[i]);
			hpSprite_[i]->Draw();
		}
	}

	// 描画終了
	Sprite::PostDraw();
}

void HUD::DrawStageNumber(int stageNo) {
	KamataEngine::DirectXCommon* dxCommon = KamataEngine::DirectXCommon::GetInstance();

	// 描画開始
	Sprite::PreDraw(dxCommon->GetCommandList());

	// 1. 「STAGE 1-」の画像を表示
	// 画面中央より少し左に配置（座標は調整してください）
	Vector2 textPos = {400.0f, 300.0f};
	stageTextSprite_->SetPosition(textPos);
	stageTextSprite_->Draw();

	// 2. 数字の画像を表示
	// stageNo に対応する数字画像を表示します。

	// 画像の幅（数字の表示位置をずらすため）
	float numberWidth = 64.0f; // ※実際の画像の幅に合わせてください

	// 「STAGE 1-」の右側に数字を表示
	Vector2 numPos = {textPos.x + 340.0f, textPos.y}; // 文字の横幅分ずらす

	// ステージ番号が2桁の場合（例: 10）の対応
	if (stageNo >= 10) {
		// 10の位
		int digit10 = stageNo / 10;
		numberSprites_[digit10]->SetPosition(numPos);
		numberSprites_[digit10]->Draw();

		// 1の位の位置へずらす
		numPos.x += numberWidth;

		// 1の位
		int digit1 = stageNo % 10;
		numberSprites_[digit1]->SetPosition(numPos);
		numberSprites_[digit1]->Draw();
	} else {
		// 1桁の場合 (0-9)
		numberSprites_[stageNo]->SetPosition(numPos);
		numberSprites_[stageNo]->Draw();
	}

	Sprite::PostDraw();
}