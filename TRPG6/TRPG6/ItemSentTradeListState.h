#pragma once
#include "IGameState.h"
#include "Player.h"

class ItemSentTradeListState : public IGameState
{
public:
    virtual void Enter() override;
    virtual void Update(int ch, std::string& lastCommand) override;

private:
    Player* player;
};
