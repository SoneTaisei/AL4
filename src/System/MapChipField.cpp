#include "MapChipField.h"
#include <fstream>
#include <map>
#include <sstream>
#include <algorithm>

using namespace KamataEngine;

namespace {

std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
    {"2", MapChipType::kPlayerStart}, // CSVの"2"をプレイヤー出現ポイントにマップ
    {"3", MapChipType::kEnemy},       // CSVの"3"を敵出現ポイントにマップ
    {"4", MapChipType::kChasingEnemy}, // CSVの"4"を追尾する敵にマップ
    {"5", MapChipType::kShooter},     // CSVの"5"を射撃する敵にマップ
    {"6", MapChipType::kGoal},        // CSVの"6"をゴールにマップ
};
}

// トリム関数（前後の空白を削除）
static inline void TrimString(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

void MapChipField::ResetMapChipData() {
	// マップチップデータをリセット
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);
	for (std::vector<MapChipType>& mapChipDataLine : mapChipData_.data) {
		mapChipDataLine.resize(kNumBlockHorizontal);
		// デフォルトは空白にしておく
		for (MapChipType& t : mapChipDataLine) {
			t = MapChipType::kBlank;
		}
	}
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	// マップチップデータをリセット
	ResetMapChipData();

	// ファイルを開く
	std::ifstream file(filePath);
#ifdef _DEBUG
	assert(file.is_open());
#endif // _DEBUG

	if (!file.is_open()) {
		// ファイルが開けなければ何もしない（既にResetで空データになっている）
		return;
	}

	std::string line;
	uint32_t row = 0;

	// CSVの各行を読み込み、カンマで分割して列に格納する（可変長に対応）
	while (row < kNumBlockVirtical && std::getline(file, line)) {
		std::istringstream line_stream(line);
		std::string word;
		uint32_t col = 0;
		while (col < kNumBlockHorizontal && std::getline(line_stream, word, ',')) {
			TrimString(word);
			if (!word.empty() && mapChipTable.contains(word)) {
				mapChipData_.data[row][col] = mapChipTable[word];
			} else {
				// 未定義トークンや空ならそのまま kBlank（Resetで既に設定済み）
			}
			++col;
		}
		// 行の列数が kNumBlockHorizontal 未満なら残りはデフォルト（kBlank）
		++row;
	}
	// ファイルの行数が kNumBlockVirtical 未満でも、Resetで空白が入っているため問題なし
	file.close();
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	// 安全な範囲チェック（符号付き比較は不要）
	if (xIndex >= kNumBlockHorizontal || yIndex >= kNumBlockVirtical) {
		return MapChipType::kBlank;
	}
	return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
	//                                                         簡易的な座標変換
	return Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVirtical - 1 - yIndex ), 0);
}

uint32_t MapChipField::GetNumBlockVirtical() { return kNumBlockVirtical; }

uint32_t MapChipField::GetNumBlockHorizontal() { return kNumBlockHorizontal; }

 // MapChipField::GetMapChipIndexSetByPosition の実装
MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const KamataEngine::Vector3 position) {
	IndexSet indexSet{};

	// X番号の計算 (変更なし)
	indexSet.xIndex = static_cast<uint32_t>(std::floor((position.x + kBlockWidth / 2.0f) / kBlockWidth));

	// Y番号の計算
	// 1. 反転前のY番号を計算
	// ワールド座標のYにブロック高さの半分を足して、ブロック高さで割る
	// ここに微小な正の値を加算することで、わずかな浮き上がりを考慮し、誤って下のブロックのインデックスを取得するのを防ぐ
	const float kEpsilon = 0.01f;                                                                                             // 例: 非常に小さい値
	uint32_t revertedYIndex = static_cast<uint32_t>(std::floor((position.y + kBlockHeight / 2.0f + kEpsilon) / kBlockHeight));

	// 2. 正しいY番号に反転させる
	indexSet.yIndex = kNumBlockVirtical - 1 - revertedYIndex;

	return indexSet;
}

// MapChipField::GetRectByIndex の実装
MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
	// 指定ブロックの中心座標を取得する
	KamataEngine::Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left = center.x - kBlockWidth / 2.0f;
	rect.right = center.x + kBlockWidth / 2.0f;   // 右端は中心 + 半幅
	rect.bottom = center.y - kBlockHeight / 2.0f; // 下端は中心 - 半高さ
	rect.top = center.y + kBlockHeight / 2.0f;    // 上端は中心 + 半高さ

	return rect;
}

// 指定タイプの最初のマップチップインデックスを探す実装
bool MapChipField::FindFirstIndexByType(MapChipType type, IndexSet& outIndex) {
	for (uint32_t y = 0; y < kNumBlockVirtical; ++y) {
		for (uint32_t x = 0; x < kNumBlockHorizontal; ++x) {
			if (mapChipData_.data[y][x] == type) {
				outIndex.xIndex = x;
				outIndex.yIndex = y;
				return true;
			}
		}
	}
	return false;
}
