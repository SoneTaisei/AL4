#include "MapChipField.h"
#include <fstream>
#include <map>
#include <sstream>

using namespace KamataEngine;

namespace {

std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
};
}

void MapChipField::ResetMapChipData() {
	// マップチップデータをリセット
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);
	for (std::vector<MapChipType>& mapChipDataLine : mapChipData_.data) {
		mapChipDataLine.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	// マップチップデータをリセット
	ResetMapChipData();

	// ファイルを開く
	std::ifstream file;
	file.open(filePath);
#ifdef _DEBUG
	assert(file.is_open());
#endif // _DEBUG

	// マップチップCSV
	std::stringstream mapChipCsv;
	// ファイルの内容を文字列ストリームにコピー
	mapChipCsv << file.rdbuf();
	// ファイルを閉じる
	file.close();

	// CSVからマップチップデータを読み込む
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		std::string line;
		// ステージの1行を読み込む
		getline(mapChipCsv, line);

		// 1行分の文字列をストリームに変換して解析しやすくする
		std::istringstream line_stream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			std::string word;
			getline(line_stream, word, ',');

			if (mapChipTable.contains(word)) {
				// 読み取った数字をマップチップデータに入れていく
				mapChipData_.data[i][j] = mapChipTable[word];
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex < 0 || kNumBlockHorizontal - 1 < xIndex) {
		return MapChipType::kBlank;
	}
	if (yIndex < 0 || kNumBlockVirtical - 1 < yIndex) {
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
	uint32_t revertedYIndex = static_cast<uint32_t>(std::floor((position.y + kBlockHeight / 2.0f + kEpsilon) / kBlockHeight)); // ★ kEpsilon を追加

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
