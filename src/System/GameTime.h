#pragma once
class GameTime {
public:
	static float GetDeltaTime();
	static void Update();

private:
	static const float deltaTime_;
};
