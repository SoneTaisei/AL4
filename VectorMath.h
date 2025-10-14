#pragma once
#include "KamataEngine.h" // Vector3などの定義が必要なため
#include <cmath>

// Vector3の正規化
inline KamataEngine::Vector3 Normalize(const KamataEngine::Vector3& v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length != 0.0f) {
		return {v.x / length, v.y / length, v.z / length};
	}
	return v; // ゼロベクトルの場合はそのまま返す
}

// Vector2の長さを計算
inline float Length(const KamataEngine::Vector2& v) { 
	return std::sqrt(v.x * v.x + v.y * v.y); 
}
