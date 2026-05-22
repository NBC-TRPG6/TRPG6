#include "BossMonster.h"

const std::string BossMonster::BossName = "대래래래래곤~~~";

std::string BossMonster::WhoAmI()
{
    return std::string();
}

Monster* BossMonster::ResetState(int playerLevel) 
{
    Name = BossName;
    Hp = RandomHp(playerLevel);
    Attack = RandomAttack(playerLevel);
    Money = RandomMoney(playerLevel);
    return this;
}
