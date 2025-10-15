#include "Easing.h"
float EaseOutQuint(float t) {
	float f = t - 1.0f;
	return f * f * f * f * f + 1.0f;
}

float Lerp(float start, float end, float t) {
	return start + (end - start) * t; 
}
