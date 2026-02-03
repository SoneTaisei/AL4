#pragma once

// ステージのクリア状態を管理する構造体
struct StageData {
	// staticにすることで、どのシーンからアクセスしても同じ実体を参照できる
	static inline bool isCleared[3] = {true, false, false};
};
