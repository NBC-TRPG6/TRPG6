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

/*
 * @brief 아이템 판매 금액 반환 메서드
 * @return MONSTER_PART 타입이면 금액 그대로, 다른 다입일 경우 구매 금액의 60%
 */
int Item::GetSellPrice() const
{
    if (type == ItemType::MONSTER_PART) return price;
    return static_cast<int>(price * 0.6);
}



