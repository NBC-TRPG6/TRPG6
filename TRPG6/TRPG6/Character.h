#pragma once
#include <string>

class Character {
public:
    Character(const std::string& name)
        : Name(name), Hp(0), Attack(0), Money(0) {
    }
    //Name,Hp,Attack,Defecse,Level

    Character(const std::string& name, int hp, int attack, int Money)
        : Name(name), Hp(hp), Attack(attack), Money(Money) {
    }

    virtual ~Character() {}

    virtual std::string WhoAmI() = 0;

    std::string Name;

    // Getters
    std::string GetName() const { return Name; }
    int GetHp() const { return Hp; }
    int GetAttack() const { return Attack; }
    int GetMoney() const { return Money; }


    // Setters
    void SetHp(int hp) {
        if (hp < 0) Hp = 0;
        else Hp = hp;
    }
    void SetAttack(int attack) { Attack = (attack < 0) ? 0 : attack; }
    void SetMoney(int money) { Money = (money < 0) ? 0 : money; }

    bool IsDead() const { return Hp <= 0; }


protected:
    int Hp;
    int Attack;
    int Money;
};
