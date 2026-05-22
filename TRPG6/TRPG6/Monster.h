#pragma once
#include "Character.h"
#include "Item.h"
#include <string>


class Monster : public Character
{
protected:

    static std::string RandomName();

    static int RandomState(int pLevel, int min, int max);

    static int RandomHp(int pLevel, float multi = 1);
    static int RandomAttack(int pLevel, float multi = 1);
    static int RandomMoney(int pLevel, float multi = 1);

    static std::string MonsterNames[10];
    static std::string MonsterItems[11];

public:

#pragma region 생성자s

    /// <summary>
    /// 플레이어 레벨을 기반으로 몬스터의 이름, 체력, 공격력, 돈을 랜덤하게 생성하여 초기화하는 생성자입니다.
    /// </summary>
    /// <param name="playerLevel"></param>
    Monster(int playerLevel = 1) :
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

    /// <summary>
    /// 플레이어 레벨과 이름을 기반으로 몬스터의 체력, 공격력, 돈을 랜덤하게 생성하여 초기화하는 생성자입니다.
    /// </summary>
    /// <param name="playerLevel"></param>
    /// <param name="name"></param>
    Monster(int playerLevel, std::string name) :
        Character(
            name,
            RandomHp(playerLevel),
            RandomAttack(playerLevel),
            RandomMoney(playerLevel))
    {}

    Monster(int playerLevel, float multiyState = 1):
        Character(
            RandomName(),
            RandomHp(playerLevel, multiyState),
            RandomAttack(playerLevel, multiyState),
            RandomMoney(playerLevel, multiyState))
    {}

    Monster(int playerLevel,std::string name, float multiyState = 1) :
        Character(
            name,
            RandomHp(playerLevel, multiyState),
            RandomAttack(playerLevel, multiyState),
            RandomMoney(playerLevel, multiyState))
    {}

#pragma endregion

    virtual ~Monster() {}

    std::string WhoAmI() override;

    /// <summary>
    /// 기존 Monster의 상태를 초기화하여 새로운 상태로 만들어 반환합니다.
    /// </summary>
    /// <param name="playerLevel"></param>
    /// <returns>새로운 상태로 초기화된 Monster 객체의 포인터</returns>
    virtual Monster* ResetState(int playerLevel);

    /// <summary>
    /// 일정 확률로 아이템을 드롭하는 메서드입니다. 아이템이 드롭될 경우 해당 아이템의 포인터를 반환하며,
    /// 드롭되지 않을 경우 nullptr을 반환합니다.
    /// </summary>
    /// <param name="percent">아이템 드롭 확률(기본값: 30%)</param>
    /// <returns>드롭된 아이템의 포인터 또는 nullptr</returns>
    Item* DropItem(int percent = 30);

};

