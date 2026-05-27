#include "Player.h"
#include "Renderer.h"

void Player::GainExp(int amount)
{
    Exp += amount;
    while (Exp >= MaxExp && Level < 10) {
        Exp -= MaxExp;
        LevelUp();
    }
}

void Player::LevelUp()
{
    if (Level < 10)	//레벨이 10 미만이면 실행
    {
        //레벨업 및 레벨업 출력
        int bfLevel = Level; //이전레벨 저장
        Level++;
        Renderer::DisplayUITimed(UIPart::CenterLeft, 1, " 레벨 업! ",3.0f); //레벨업 문구 출력
        Renderer::DisplayUITimed(UIPart::CenterLeft, 2, "Lv." + std::to_string(bfLevel) + " -> Lv." + std::to_string(Level), 3.0f); //레벨 변화 출력

        //능력치 상승
        int bfHP = MaxHp; //이전 최대 체력 저장
        int bfAttack = Attack; //이전 공격력 저장
        MaxHp += Level * 20; //현재 (레벨*20)만큼 최대HP 증가
        Hp = MaxHp; //최대 HP만큼 HP 회복
        Attack += Level * 5; //공격력 (레벨*5)만큼 증가
        Renderer::DisplayUITimed(UIPart::CenterLeft, 3, "최대 체력: " + std::to_string(bfHP) + " -> " + std::to_string(MaxHp), 3.0f); //최대 체력 변화 출력
        Renderer::DisplayUITimed(UIPart::CenterLeft, 4, "공격력: " + std::to_string(bfAttack) + " -> " + std::to_string(Attack),3.0f); //공격력 변화 출력
    }
}


void Player::EquipWeapon(WeaponItem* weapon)
{
    // 기존 무기 해제
    if (equippedWeapon != nullptr)
    {
        Attack -= equippedWeapon->GetValue();
        MaxHp -= equippedWeapon->GetHPBonus();
        Hp -= equippedWeapon->GetHPBonus();
    }

    // 새 무기 장착
    equippedWeapon = weapon;
    if (weapon != nullptr)
    {
        Attack += weapon->GetValue();
        MaxHp += weapon->GetHPBonus();
        Hp += weapon->GetHPBonus();
    }
}

bool Player::RemoveItem(const std::string& itemName, int amount)
{
    // 장착 무기면 먼저 해제
    if (equippedWeapon != nullptr && equippedWeapon->GetName() == itemName)
    {
        EquipWeapon(nullptr);
    }
    return inventory.RemoveItem(itemName, amount);
}


void Player::PrintStatus() const
{
    Renderer::DisplayUI(UIPart::CenterRight, 0, "[ 캐릭터 정보 ]");
    Renderer::DisplayUI(UIPart::CenterRight, 1, "이름: " + Name);
    Renderer::DisplayUI(UIPart::CenterRight, 2, "레벨: " + std::to_string(Level));
    Renderer::DisplayUI(UIPart::CenterRight, 3, "경험치: " + std::to_string(Exp) + "/100");
    Renderer::DisplayUI(UIPart::CenterRight, 4, "HP: " + std::to_string(Hp) + "/" + std::to_string(MaxHp));
    Renderer::DisplayUI(UIPart::CenterRight, 5, "공격력: " + std::to_string(Attack));
    Renderer::DisplayUI(UIPart::CenterRight, 6, "보유 골드: " + std::to_string(Money));
    for (const auto& invSlot : inventory.GetSlots())
    {
        if (invSlot.item == equippedWeapon && invSlot.count > 0)
        {
            Renderer::DisplayUI(UIPart::CenterRight, 7, invSlot.item->GetName());
            break;
        }
    }
}
