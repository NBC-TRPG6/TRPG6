#include "Player.h"
#include "Renderer.h"

void Player::GainExp(int amount)
{
    Exp += amount;
    while (Exp > MaxExp && Level < 10) {
        Exp -= MaxExp;
        LevelUp();
    }
}

void Player::LevelUp()
{
    Level++;
    MaxHp += Level * 20;
    Attack += Level * 5;
    Defense += Level * 5;
    Hp = MaxHp;
}

void Player::PrintStatus() const
{
    Renderer::DisplayUI(UIPart::CenterRight, 0, "[ 캐릭터 정보 ]");
    Renderer::DisplayUI(UIPart::CenterRight, 1, "이름: " + Name);
    Renderer::DisplayUI(UIPart::CenterRight, 2, "레벨: " + std::to_string(Level) + "경험치: " + std::to_string(Exp) + "/100");
    Renderer::DisplayUI(UIPart::CenterRight, 3, "HP: " + std::to_string(Hp) + "/" + std::to_string(MaxHp));
    Renderer::DisplayUI(UIPart::CenterRight, 4, "공격력: " + std::to_string(Attack) + "방어력: " + std::to_string(Defense));
}
