#pragma once
#include <string>
#include "IGameState.h"
#include "BattleManager.h"

class BossBattleState : public IGameState
{
public:
    virtual ~BossBattleState() override
    {
        delete bossMonster;
    }

    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;

private:
    BattleManager battleManager;
    Monster* bossMonster;
    Player* player;
};

