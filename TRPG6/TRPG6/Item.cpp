#include "Item.h"
#include "Character.h"
#include "cmath"
#include "iostream"
#include "IPCManager.h"
#include <utility>

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
        target->SetHp(target->GetHp() + target->GetMaxHp()*value/100);
        break;
    case ItemType::ATTACK_BUFF: {

        int currentAttack = target->GetAttack();
        int bonusAttack = std::round(currentAttack * 4 / 100.0f);
        IPCManager::GetInstance().SendLog("[아이템 사용됨] 현재 공격력: " + std::to_string(currentAttack)+ " | 계산된 증가량: " + std::to_string(bonusAttack)+ " | 결과: " + std::to_string(currentAttack + bonusAttack));

        target->SetAttack(currentAttack + bonusAttack);
        break;
    }
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



