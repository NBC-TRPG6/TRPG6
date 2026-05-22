#include "BossMonster.h"

const std::string BossMonster::BossName = "대래래래래곤~~~";

std::string BossMonster::WhoAmI()
{
    return Name;
}

Monster* BossMonster::ResetState(int playerLevel) 
{
    Name = BossName;
    Hp = RandomHp(playerLevel,1.5f);
    Attack = RandomAttack(playerLevel, 1.5f);
    Money = RandomMoney(playerLevel, 1.5f);
    return this;
}
