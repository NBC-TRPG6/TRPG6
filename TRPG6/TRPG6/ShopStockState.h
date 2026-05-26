#pragma once
#include "IGameState.h"

class Player;

class ShopStockState : public IGameState
{
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;

private:
    Player* player;
};
