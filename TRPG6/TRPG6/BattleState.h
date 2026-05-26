#pragma once
#include <string>
#include "IGameState.h"
#include "BattleManager.h"

class BattleState : public IGameState
{
public:
    virtual ~BattleState() override
    {
        delete bossMonster;
    }

    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
    void Exit() override {}

private:
    BattleManager battleManager;
    Monster* bossMonster;
    Player* player;
    int TurnCount =0;
};

