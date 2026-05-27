#include "WeaponItem.h"
#include "WeaponTable.h"

std::string WeaponItem::GetColoredName() const
{
    return WeaponTable::GetInstance().GetRarityColor(upgradeCount) + name + "\033[0m";
}
