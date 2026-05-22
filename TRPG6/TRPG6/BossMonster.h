#pragma once
#include "Monster.h"

class BossMonster : public Monster
{
private:
    static const std::string BossName;
public:
    BossMonster(int playerLevel = 1) : Monster(playerLevel, BossName,1.5f) {}

    std::string WhoAmI() override;
    virtual Monster* ResetState(int playerLevel) override;
};

