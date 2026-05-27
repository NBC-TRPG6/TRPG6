#pragma once
#include "Item.h"
#include <string>
#include <iostream>


class WeaponItem : public Item
{
public:
    WeaponItem(const std::string& name, const std::string& baseName, int upgradeCount, int attackBonus, int hpBonus, int price)
        : Item(name, ItemType::WEAPON, attackBonus, price)
        , baseName(baseName)
        , upgradeCount(upgradeCount)
        , hpBonus(hpBonus) {
    }


    int GetHPBonus() const { return hpBonus; }
    std::string GetBaseName() const { return baseName; }
    int GetUpgradeCount() const { return upgradeCount; }
    std::string GetColoredName() const;

private:
    std::string baseName;   // 무기 종류 (예: "검", "도끼")
    int upgradeCount;       // 강화 단계 (0~4)
    int hpBonus;            // HP 보너스

};



//TODO:: 장비 장착 시 모든 UI가 색이 변함
