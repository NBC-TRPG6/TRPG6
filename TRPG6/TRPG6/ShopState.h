#pragma once
#include "IGameState.h"
#include "Player.h"

class ShopState : public IGameState
{
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;

private:
    Player* player;
};
