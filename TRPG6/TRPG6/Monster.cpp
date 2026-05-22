#include "Monster.h"

std::string Monster::RandomName()
{
    std::string MonsterNames[10] = {
           "고블린", "오크", "트롤", "스켈레톤", "좀비",
           "뱀파이어", "늑대인간", "거미", "슬라임", "산적"
    };
    return MonsterNames[0 + rand() % (9 - 0 + 1)];
}

int Monster::RandomState(int pLevel, int min, int max)
{
    return pLevel * (min + rand() % (max - min + 1));
}

int Monster::RandomHp(int pLevel)
{
    return RandomState(pLevel, 20, 30);
}

int Monster::RandomAttack(int pLevel)
{
    return RandomState(pLevel, 5, 10);
}

int Monster::RandomMoney(int pLevel)
{
    return RandomState(pLevel, 5, 10);
}

std::string Monster::WhoAmI()
{
    return "Monster";
}


Monster* Monster::ResetState(int playerLevel)
{
    Name = RandomName();
    Hp = RandomHp(playerLevel);
    Attack = RandomAttack(playerLevel);
    Money = RandomMoney(playerLevel);
    return this;
}

