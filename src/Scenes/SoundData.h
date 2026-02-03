#pragma once
#include <cstdint>
#include "KamataEngine.h"
class SoundData {
public:
	// ステージクリアフラグ
	static inline bool isCleared[3] = {false, false, false};

	// --- BGM ハンドル ---
	static inline uint32_t bgmTitle = 0u;
	static inline uint32_t bgmGame = 0u;
	static inline uint32_t bgmVoiceHandle = 0u; // ループ停止用

	// --- SE ハンドル ---
	static inline uint32_t seClear = 0u;
	static inline uint32_t seEnemyDeath = 0u;
	static inline uint32_t seMoveSelect = 0u;
	static inline uint32_t sePlayerAttack = 0u;
	static inline uint32_t sePlayerDamage = 0u;
	static inline uint32_t sePlayerJump = 0u;
	static inline uint32_t seSelect = 0u;
};
