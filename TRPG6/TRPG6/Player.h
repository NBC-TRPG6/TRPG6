#pragma once
#include <string>
#include "Character.h"
#include "Inventory.h"
#include "WeaponItem.h"
#include "WeaponTable.h"

class Player : public Character {
private:
    Inventory<Item> inventory;
    //장착 장비
    WeaponItem* equippedWeapon = nullptr;

public:
    Player(const std::string& name)
        : Character(name), Exp(0), MaxExp(100), Level(1) {
        Hp = 200;
        MaxHp = Hp;
        Attack = 30;
    }
    Inventory<Item>& GetInventory() { return inventory; }

    virtual ~Player() {}

    virtual std::string WhoAmI() { return "Player"; }


    int Exp;
    int MaxExp;
    int Level;


    // Getters

    int GetExp() const { return Exp; }
    int GetMaxExp() const { return MaxExp; }
    int GetLevel() const { return Level; }
    WeaponItem* GetEquippedWeapon() const { return equippedWeapon; }

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

    //무기 장착,해제 함수
    void EquipWeapon(WeaponItem* weapon);
    //아이템 삭제 함수(강화,조합등에서 사용)
    bool RemoveItem(const std::string& itemName, int amount = 1);
};
