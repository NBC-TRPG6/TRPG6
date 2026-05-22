#pragma once
#include <string>
#include "Character.h"
#include "Inventory.h"

class Player : public Character {
private:
    Inventory<Item> inventory;
public:
    Player(const std::string& name)
        : Character(name), MaxHp(200), Exp(0), MaxExp(100), Level(1) {
        Hp = 200;
        Attack = 30;
        Defense = 10;
    }
    Inventory<Item>& GetInventory() { return inventory; }

    virtual ~Player() {}

    virtual std::string WhoAmI() { return "Player"; }


    std::string Name;
    int MaxHp;
    int Exp;
    int MaxExp;
    int Level;


    // Getters
    int GetMaxHp() const { return MaxHp; }
    int GetExp() const { return Exp; }
    int GetMaxExp() const { return MaxExp; }
    int GetLevel() const { return Level; }

    // Setters        
    void SetHp(int hp) {
        if (hp > MaxHp) Hp = MaxHp;
        else if (hp < 0) Hp = 0;
        else Hp = hp;
    }

    void SetLevel(int level) { Level = (level < 1) ? 1 : (level > 10) ? 10 : level; }

    void GainExp(int amount);
    void LevelUp();

    void PrintStatus() const;


protected:
    int Hp;
    int Attack;
    int Defense;
};
