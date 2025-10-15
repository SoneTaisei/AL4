#pragma once
#include"KamataEngine.h"

/// <summary>///
/// 行列を計算・転送する
/// </summary>///
/// <param name="worldTransforme"></param>
class TransformUpdater {
public:
	static void WorldTransformUpdate(KamataEngine::WorldTransform& worldTransforme);

	// Z軸回転行列の作成
	static KamataEngine::Matrix4x4 MakeRoteZMatrix(float radian);

	// ベクトルの座標変換
	static KamataEngine::Vector3 Transform(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& matrix);

	/// <summary>
	/// ビュー行列（LookAt）を生成する
	/// </summary>
	static KamataEngine::Matrix4x4 MakeLookAtMatrix(const KamataEngine::Vector3& eye, const KamataEngine::Vector3& target, const KamataEngine::Vector3& up);

	/// <summary>
	/// ベクトルを座標変換する（平行移動の影響を受けない）
	/// </summary>
	static KamataEngine::Vector3 TransformNormal(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& matrix);

};