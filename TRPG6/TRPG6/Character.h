#pragma once
#include <string>

class Character {
public:
    Character(const std::string& name, int hp, int mp, int attack, int defense)
        : Name(name), Hp(hp), Mp(mp), Attack(attack), Defense(defense), MaxHp(hp), MaxMp(mp) {
    }

    virtual ~Character() {}

    virtual std::string WhoAmI() = 0;

    std::string Name;
    int MaxHp;
    int Mp;
    int MaxMp;

    // Getters
    int GetHp() const { return Hp; }
    int GetAttack() const { return Attack; }
    int GetDefense() const { return Defense; }

    // Setters        
    void SetHp(int value) {
        if (value > MaxHp) Hp = MaxHp;
        else if (value < 0) Hp = 0;
        else Hp = value;
    }

    // 공격력 설정: 음수 방지
    void SetAttack(int value) {
        Attack = (value < 0) ? 0 : value;
    }

    // 방어력 설정: 음수 방지
    void SetDefense(int value) {
        Defense = (value < 0) ? 0 : value;
    }

protected:
    int Hp;
    int Attack;
    int Defense;
};