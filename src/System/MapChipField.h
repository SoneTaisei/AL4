#pragma once
#include "KamataEngine.h"
/// <summary>
/// マップチップフィールド
/// </summary>

enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
	kPlayerStart, // プレイヤー出現ポイント (CSVの"2")
	kEnemy, // 敵出現ポイント (CSVの"3")
	kChasingEnemy, // 追尾する敵 (CSVの"4")
	kShooter, // 射撃する敵 (CSVの"5")
	kGoal, // ゴール (CSVの"6")
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField {
private:
	// 1ブロックのサイズ
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;
	// ブロックの個数
	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;

	// マップチップのデータ
	MapChipData mapChipData_;

public:

	// マップチップのインデックスを格納する構造体
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};

	// 範囲矩形
	struct Rect {
		float left;   // 左端
		float right;  // 右端
		float bottom; // 下端
		float top;    // 上端
	};

	void ResetMapChipData();

	void LoadMapChipCsv(const std::string& filePath);

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	KamataEngine::Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	//ブロックの数のゲッター
	uint32_t GetNumBlockVirtical();
	uint32_t GetNumBlockHorizontal();

	/// <summary>
	/// 座標からマップチップ番号を計算
	/// </summary>
	/// <param name="position">ワールド座標</param>
	/// <returns>マップチップのインデックス</returns>
	IndexSet GetMapChipIndexSetByPosition(const KamataEngine::Vector3 position);

	// kBlockHeight のゲッター
    float GetBlockHeight() const { return kBlockHeight; }
    // kBlockWidth のゲッター (必要に応じて)
    float GetBlockWidth() const { return kBlockWidth; }

	/// <summary>
	/// マップチップ番号を指定して、ブロックの全方向の境界座標を得る関数
	/// </summary>
	/// <param name="xIndex">Xインデックス</param>
	/// <param name="yIndex">Yインデックス</param>
	/// <returns>ブロックの境界範囲</returns>
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

	/// <summary>
	/// 指定した MapChipType を持つ最初のインデックスを検索する
	/// 見つかれば true を返し outIndex に格納する。見つからなければ false を返す。
	/// </summary>
	bool FindFirstIndexByType(MapChipType type, IndexSet& outIndex);
};
