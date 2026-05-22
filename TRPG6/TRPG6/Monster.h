#pragma once
#include "Character.h"
#include <string>


class Monster : public Character
{
protected:
    static std::string RandomName();

    static int RandomState(int pLevel, int min, int max);

    static int RandomHp(int pLevel);
    static int RandomAttack(int pLevel);
    static int RandomMoney(int pLevel);


public:
    /// <summary>
    /// 플레이어 레벨을 기반으로 몬스터의 이름, 체력, 공격력, 돈을 랜덤하게 생성하여 초기화하는 생성자입니다.
    /// </summary>
    /// <param name="playerLevel"></param>
    Monster(int playerLevel = 0) :
                Character(
            RandomName(),
            RandomHp(playerLevel),
            RandomAttack(playerLevel),
            RandomMoney(playerLevel))
    {}

    /// <summary>
    /// 플레이어 레벨과 돈을 기반으로 몬스터의 이름, 체력, 공격력, 돈을 랜덤하게 생성하여 초기화하는 생성자입니다.
    /// </summary>
    /// <param name="playerLevel"></param>
    /// <param name="money"></param>
    Monster(int playerLevel,int money) :
        Character(
            RandomName(),
            RandomHp(playerLevel),
            RandomAttack(playerLevel),
            money)
    {}

    Monster(int playerLevel, std::string name) :
        Character(
            name,
            RandomHp(playerLevel),
            RandomAttack(playerLevel),
            RandomMoney(playerLevel))
    {}

    virtual ~Monster() {}

    std::string WhoAmI() override;

    /// <summary>
    /// 기존 Monster의 상태를 초기화하여 새로운 상태로 만들어 반환합니다.
    /// </summary>
    /// <param name="playerLevel"></param>
    /// <returns>새로운 상태로 초기화된 Monster 객체의 포인터</returns>
    Monster* ResetState(int playerLevel);
};

