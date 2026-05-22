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
    if (Level < 10)	//레벨이 10 미만이면 실행
    {
        //레벨업 및 레벨업 출력
        int bfLevel = Level; //이전레벨 저장
        Level++;
        Renderer::DisplayUI(UIPart::CenterLeft, 0, "★★★ 레벨 업! ★★★"); //레벨업 문구 출력
        Renderer::DisplayUI(UIPart::CenterLeft, 1, "Lv." + std::to_string(bfLevel) + " -> Lv." + std::to_string(Level)); //레벨 변화 출력

        //능력치 상승
        int bfHP = MaxHp; //이전 최대 체력 저장
        int bfAttack = Attack; //이전 공격력 저장
        MaxHp += Level * 20; //현재 (레벨*20)만큼 최대HP 증가
        Hp = MaxHp; //최대 HP만큼 HP 회복
        Attack += Level * 5; //공격력 (레벨*5)만큼 증가
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "최대 체력: " + std::to_string(bfHP) + " -> " + std::to_string(MaxHp)); //최대 체력 변화 출력
        Renderer::DisplayUI(UIPart::CenterLeft, 3, "공격력: " + std::to_string(bfAttack) + " -> " + std::to_string(Attack)); //공격력 변화 출력
    }
}

void Player::PrintStatus() const
{
    Renderer::DisplayUI(UIPart::CenterRight, 0, "[ 캐릭터 정보 ]");
    Renderer::DisplayUI(UIPart::CenterRight, 1, "이름: " + Name);
    Renderer::DisplayUI(UIPart::CenterRight, 2, "레벨: " + std::to_string(Level) + "경험치: " + std::to_string(Exp) + "/100");
    Renderer::DisplayUI(UIPart::CenterRight, 3, "HP: " + std::to_string(Hp) + "/" + std::to_string(MaxHp));
    Renderer::DisplayUI(UIPart::CenterRight, 4, "공격력: " + std::to_string(Attack) + "방어력: " + std::to_string(Defense));
}
