#include "Easing.h"
#include <cmath>
float EaseOutQuint(float t) {
	float f = t - 1.0f;
	return f * f * f * f * f + 1.0f;
}

float EaseOutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }

float Lerp(float start, float end, float t) {
	return start + (end - start) * t; 
}

// EaseOutQuart (4乗の減速)
float EaseOutQuart(float t) {
	float f = t - 1.0f;
	return 1.0f - (f * f * f * f);
}

// EaseInOutQuad (前半加速・後半減速)
float EaseInOutQuad(float t) {
	if (t < 0.5f) {
		return 2.0f * t * t;
	} else {
		float f = -2.0f * t + 2.0f;
		return 1.0f - (f * f) / 2.0f;
	}
}

float EaseOutBack(float t) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;

	// t は 0.0 ～ 1.0 の範囲
	return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}
