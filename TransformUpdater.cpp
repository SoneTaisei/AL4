#include "TransformUpdater.h"

using namespace KamataEngine;

static Matrix4x4 MakeRoteXMatrix(float radian) {
	Matrix4x4 result{};

	result = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, std::cos(radian), std::sin(radian), 0.0f, 0.0f, -std::sin(radian), std::cos(radian), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

	return result;
}

static Matrix4x4 MakeRoteYMatrix(float radian) {
	Matrix4x4 result{};

	result = {std::cos(radian), 0.0f, -std::sin(radian), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, std::sin(radian), 0.0f, std::cos(radian), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

	return result;
}

/*行列の計算
*********************************************************/

// 平行移動行列
static Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = {};

	result = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, translate.x, translate.y, translate.z, 1.0f};

	return result;
}

Matrix4x4 TransformUpdater::MakeRoteZMatrix(float radian) {
	Matrix4x4 result{};

	result = {std::cos(radian), std::sin(radian), 0.0f, 0.0f, -std::sin(radian), std::cos(radian), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

	return result;
}

// 拡大縮小行列
static Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result = {};

	result = {
	    scale.x, 0.0f, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 0.0f, scale.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	};

	return result;
}

static Matrix4x4 Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2) {
	Matrix4x4 result = {};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
			}
		}
	}

	return result;
}

// アフィン変換
static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 result{};

	// 回転行列
	Matrix4x4 rotZ = TransformUpdater::MakeRoteZMatrix(rotate.z);
	Matrix4x4 rotX = MakeRoteXMatrix(rotate.x);
	Matrix4x4 rotY = MakeRoteYMatrix(rotate.y);

	// 回転行列の合成
	Matrix4x4 rotateMatrix = Multiply(Multiply(rotX, rotY), rotZ);

	// 拡大縮小
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	// 平行移動
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	result = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);

	return result;
}

void TransformUpdater::WorldTransformUpdate(WorldTransform& worldTransforme) {
	// スケール、回転、平行移動を合成して行列を計算する
	worldTransforme.matWorld_ = MakeAffineMatrix(worldTransforme.scale_, worldTransforme.rotation_, worldTransforme.translation_);
	// 定数バッファへの書き込み
	worldTransforme.TransferMatrix();
}

// Transform 関数を TransformUpdater の静的メンバ関数として実装
Vector3 TransformUpdater::Transform(const Vector3& vector, const Matrix4x4& matrix) {
	Vector3 result;
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];
#ifdef _DEBUG
	// wが0の可能性がある場合はassertで止める
	assert(w != 0.0f);
#endif // _DEBUG
	// wで割って同次座標をデカルト座標に戻す
	result.x /= w;
	result.y /= w;
	result.z /= w;
	return result;
}

// ベクトルの正規化
static KamataEngine::Vector3 Normalize(const KamataEngine::Vector3& v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length != 0.0f) {
		return {v.x / length, v.y / length, v.z / length};
	}
	return v;
}

// ベクトルの外積
static KamataEngine::Vector3 Cross(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) { return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x}; }

// ビュー行列（LookAt）を生成する関数の実装
KamataEngine::Matrix4x4 TransformUpdater::MakeLookAtMatrix(const KamataEngine::Vector3& eye, const KamataEngine::Vector3& target, const KamataEngine::Vector3& up) {
	Vector3 zaxis = Normalize(target - eye);
	Vector3 xaxis = Normalize(Cross(up, zaxis));
	Vector3 yaxis = Cross(zaxis, xaxis);

	Matrix4x4 result = {};
	result.m[0][0] = xaxis.x;
	result.m[0][1] = yaxis.x;
	result.m[0][2] = zaxis.x;
	result.m[0][3] = 0;
	result.m[1][0] = xaxis.y;
	result.m[1][1] = yaxis.y;
	result.m[1][2] = zaxis.y;
	result.m[1][3] = 0;
	result.m[2][0] = xaxis.z;
	result.m[2][1] = yaxis.z;
	result.m[2][2] = zaxis.z;
	result.m[2][3] = 0;
	result.m[3][0] = -(eye.x * xaxis.x + eye.y * xaxis.y + eye.z * xaxis.z);
	result.m[3][1] = -(eye.x * yaxis.x + eye.y * yaxis.y + eye.z * yaxis.z);
	result.m[3][2] = -(eye.x * zaxis.x + eye.y * zaxis.y + eye.z * zaxis.z);
	result.m[3][3] = 1;

	return result;
}

// TransformNormal 関数の実装
KamataEngine::Vector3 TransformUpdater::TransformNormal(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& matrix) {
	Vector3 result;
	// 平行移動成分（4行目やw）を無視して、回転と拡縮だけを適用する
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2];
	return result;
}
