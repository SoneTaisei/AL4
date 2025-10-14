#pragma once
#include "KamataEngine.h"
/// <summary>
/// AABB (Axis-Aligned Bounding Box)
/// </summary>
struct AABB {
	KamataEngine::Vector3 min; // 最小点
	KamataEngine::Vector3 max; // 最大点
};
