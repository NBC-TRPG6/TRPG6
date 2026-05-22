#include "Item.h"
#include "Character.h"

/*
 * @brief 아이템 효과 플레이어에게 적용하는 메서드
 * @param target 플레이어
 */
void Item::Use(Character* target)
{
    if (!target) { return; }

    switch (type)
    {
    case ItemType::HP_POTION:
        target->SetHp(target->GetHp() + value);
        break;
    case ItemType::ATTACK_BUFF:
        target->SetAttack(target->GetAttack() + value);
        break;
    default:
        break;
    }
}
