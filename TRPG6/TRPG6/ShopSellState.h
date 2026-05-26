#pragma once
#include "IGameState.h"

class Player;

class ShopSellState : public IGameState
{
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;

private:
    Player* player;
};
