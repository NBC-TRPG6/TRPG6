#include "BattleManager.h"
#include "GameManager.h"



#pragma region BeginBattle

/// <summary>
/// 플레이어를 넣으면 랜덤몬스터를 불러와 전투를 시작하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
void BattleManager::StartBattle(Player& player)
{
    if (CurrentBattleState != EBattleState::Ready)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "현재는 전투를 시작할 수 없습니다.");
        return;
    }

    if (player.GetLevel() > 9)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "이제 일반 몬스터는 상대도 안 된다!");
        Monster DEARAGON(player.GetLevel(), "대래래래래곤~~~", 1.5f); // 레벨과 이름을 기반으로 보스 몬스터 생성
        Battle(player, DEARAGON); // 보스 몬스터와 전투 시작
        isBoss = true;
    }
    else
    {
        //몬스터 생성
        Monster monster(player.GetLevel(), 1.f);
        monster.ResetState(player.GetLevel()); //몬스터 상태 초기화
        Battle(player, monster); //전투 시작
    }


}

/// <summary>
/// 두 캐릭터를 넣으면 전투를 진행하는 함수입니다. 전투가 끝날 때까지 반복됩니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
void BattleManager::Battle(Player& player, Monster& monster)
{

    //배틀 상태를 InProgress로 변경합니다.
    CurrentBattleState = EBattleState::InProgress;

    //플레이어의 원래 공격력을 OriginalPlayerAttack 에 저장합니다.
    int OriginalPlayerAttack = player.GetAttack();

    //dist(0,99)는 분포로, dist(rng)가 된다면 rng의 난수 값을 0~99로 제한합니다.
    std::uniform_int_distribution<int> dist(0, 99);
    bool isPlayerTurn = dist(rng) >= 5; // 95% 확률로 플레이어가 선공, 5% 확률로 몬스터가 선공

    if (isBoss)
    {
        isPlayerTurn = false;
    }

    if (!isPlayerTurn && !isBoss)
        Renderer::DisplayUI(UIPart::CenterLeft, 2, monster.GetName() + "에게 기습당했습니다!!!");
    else if (!isPlayerTurn && isBoss)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "대래래래래곤에게 기습당할뻔했습니다!!!");
    }
    
    //전투 루프
    while (!player.IsDead() && !monster.IsDead())
    {
        if (isPlayerTurn)
            PlayerTurn(player, monster);
        else
            MonsterTurn(player, monster);
        isPlayerTurn = !isPlayerTurn;
    }

    //연속전투 막기
    CurrentBattleState = EBattleState::Locked;
    player.SetAttack(OriginalPlayerAttack); //플레이어 공격력을 원래대로 돌려놓습니다.


    //플레이어의 승리여부(사망여부)확인
    if (player.IsDead())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 8, "패배했습니다...");
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 8, "승리했습니다!");
        //플레이어 보상 받기
        BattleEnd(player, monster);
    }

}


#pragma endregion


#pragma region P&M Turn

/// <summary>
/// 플레이어 의 차례에 작동하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
void BattleManager::PlayerTurn(Player& player, Monster& monster)
{
    Renderer::DisplayUI(UIPart::CenterLeft, 3, player.GetName() + "의 턴!");
    std::uniform_int_distribution<int> dist(0, 99); //0~ 99의 균등 분포 생성

    bool isplayerUseItem = dist(rng) < 20; // 20% 확률로 아이템 사용

    if (player.GetHp() < player.GetMaxHp() / 2 && !isplayerUseItem) // 체력이 절반 이하일 때 아이템 사용 확률 증가
    {
        isplayerUseItem = dist(rng) < 50; // 50% 확률로 아이템 사용, 더블체크로 체력이 낮을 때 아이템 사용 확률 증가
    }

    if (isplayerUseItem)
    {
        if (player.GetHp() == player.GetMaxHp()) // 최대 체력이면
        {
            //player.useItem(AttackPotion) //TODO:: 최대 체력이면 무조건 공격력 포션
        }
        else if (player.GetHp() < player.GetMaxHp() / 2) // 반피 이하면
        {
            //player.useItem(HealPotion) //TODO:: 반피 이하면 무조건 회복 포션
        }
        else // 그 사이 (반피 ~ 최대 미만)
        {
            int UseItemType = dist(rng) < 50 ? 0 : 1; // 50% 확률로 공격력 또는 회복 포션
            if (UseItemType == 0)
            {
                //player.useItem(AttackPotion) //TODO
            }
            else
            {
                //player.useItem(HealPotion) //TODO
            }
        }

        Renderer::DisplayUI(UIPart::CenterLeft, 4, player.GetName() + "이 아이템을 사용했습니다!");
        // Renderer::DisplayUI(UIPart::CenterLeft, 5, player.GetName() + "이 아이템을 사용했습니다!"); TODO::사용한 아이템 출력

        return; // 아이템 사용 후 공격하지 않고 턴 종료
    }



    int PAttackDamage = player.GetAttack(); //플레이어 공격력 저장
    if (dist(rng) < 10) // 10% 확률로 크리티컬 히트
    {
        PAttackDamage *= 2;
        Renderer::DisplayUI(UIPart::CenterLeft, 4, "크리티컬 히트!!!");
    }

    //몬스터 체력 변경
    monster.SetHp(monster.GetHp() - PAttackDamage);
    Renderer::DisplayUI(UIPart::CenterLeft, 5, monster.GetName() + "의 체력이" + std::to_string(PAttackDamage) + "만큼 달았습니다!");

    //몬스터 사망 여부 확인
    if (monster.GetHp() <= 0)
    {
        monster.SetHp(0);
        Renderer::DisplayUI(UIPart::CenterLeft, 6, monster.GetName() + "가 쓰러졌습니다!");
    }
    else //안죽었다면
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 6, monster.GetName() + "의 남은 체력 : " + std::to_string(monster.GetHp()));
    }

}


/// <summary>
/// 몬스터의 차례에 작동하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
void BattleManager::MonsterTurn(Player& player, Monster& monster)
{
    Renderer::DisplayUI(UIPart::CenterLeft, 3, monster.GetName() + "의 턴!");

    std::uniform_int_distribution<int> dist(0, 99); //0~ 99의 균등 분포 생성

    int MAttackDamage = monster.GetAttack(); // 몬스터 공격력 저장
    if (dist(rng) < 10) // 10% 확률로 크리티컬 히트
    {
        MAttackDamage *= 2;
        Renderer::DisplayUI(UIPart::CenterLeft, 4, "크리티컬 히트!!!");
    }

    //플레이어 체력 변경
    player.SetHp(player.GetHp() - MAttackDamage);
    Renderer::DisplayUI(UIPart::CenterLeft, 5, player.GetName() + "의 체력이" + std::to_string(MAttackDamage) + "만큼 달았습니다!");

    //플레이어 사망 여부 확인
    if (player.GetHp() <= 0)
    {
        player.SetHp(0);
        Renderer::DisplayUI(UIPart::CenterLeft, 6, player.GetName() + "가 쓰러졌습니다!");
    }
    else //안죽었다면
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 6, player.GetName() + "의 남은 체력" + std::to_string(player.GetHp()));
    }

}

#pragma endregion


#pragma region BattleEnd


/// <summary>
/// 배틀종료 후 결과 처리 함수입니다.
/// </summary>
/// <param name="player">보상을 받을 플레이어</param>
/// <param name="monster">처치한 몬스터(돈/아이템 제공)</param>
void BattleManager::BattleEnd(Player& player, Monster& monster)
{

    //대래곤 처치시 엔딩
    if (isBoss)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "대래래래래래래래래래래래곤을 쓰러뜨렸다!!!!!!!!!!!!!!!!!!!!!!!!!");
        Renderer::DisplayUI(UIPart::CenterLeft, 3, "십억을 받았습니다.");
        Renderer::DisplayUI(UIPart::CenterLeft, 4, "-끝-");
        quick_exit(0); // 게임 종료
        return;
    }

    //몬스터의 소지금 강탈
    player.SetMoney(player.GetMoney() + monster.GetMoney());

    player.GainExp(50); // 경험치 50 획득

    Item* item = monster.DropItem(); //30%확률로 아이템 드랍

    //TODO:: 몬스터가 주는 아이템을 플레이어 인벤토리에 추가(함수 필요, 내함수 X)



    /*
    //TODO:: 배틀 후 획득한 것 표시
    Renderer::DisplayUI(UIPart::CenterLeft, 6, player.GetName() + "의 남은 체력" + std::to_string(player.GetHp()));
    Renderer::DisplayUI(UIPart::CenterLeft, 6, player.GetName() + "의 남은 체력" + std::to_string(player.GetHp()));
    Renderer::DisplayUI(UIPart::CenterLeft, 6, player.GetName() + "의 남은 체력" + std::to_string(player.GetHp()));
    */


}

#pragma endregion

