#pragma once

class Player;

class UI {
public:
	void Initialize();

	void Update();

	void Draw(const Player* player);
};
