#include "BattleManager.h"
#include "GameManager.h"
#include "IPCManager.h"
#include "Item.h"


#pragma region BeginBattle

/// <summary>
/// 플레이어를 넣으면 랜덤몬스터를 불러와 전투를 시작하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
void BattleManager::StartBattle(Player* player)
{
    //dist(0,99)는 분포로, dist(rng)가 된다면 rng의 난수 값을 0~99로 제한합니다.
    std::uniform_int_distribution<int> dist(0, 99);
    int nimochance = dist(rng);
    isNIMO = false;
    useItemName = "";

    bool isPlayerTurn = dist(rng) >= 5; // 95% 확률로 플레이어가 선공, 5% 확률로 몬스터가 선공

    if (CurrentBattleState != EBattleState::Ready)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "현재는 전투를 시작할 수 없습니다.");
        return;
    }
    //전투 상태 전환
    CurrentBattleState = EBattleState::InProgress;
    OriginalPlayerAttack = player->GetAttack(); //플레이어의 원래 공격력을 저장합니다.

    //if (player->GetLevel() > 9)
    //{
    //    Renderer::DisplayUI(UIPart::CenterLeft, 2, "이제 일반 몬스터는 상대도 안 된다!");
    //    Monster DEARAGON(player->GetLevel(), "대래래래래곤~~~", 1.5f); // 레벨과 이름을 기반으로 보스 몬스터 생성
    //    isBoss = true;
    //    isPlayerTurn = false; // 보스몬스터의 선공으로 시작
    //    currentMonster = DEARAGON; // 현재 몬스터로 보스몬스터 설정
    //}

    if (nimochance <= 2 && !NimoDefeated)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "!!!!!!!야생의 니모가 나타났다!!!!!!!!");
        Monster NIMO(player->GetLevel(), "니모", 0.5f); // 체공이 일반몹보다 낮다.
        currentMonster = NIMO; // 현재 몬스터로 니모 설정
        isPlayerTurn = false;
        isNIMO = true;

    }
    else
    {
        //몬스터 생성
        Monster monster(player->GetLevel(), 1.f);
        monster.ResetState(player->GetLevel()); //몬스터 상태 초기화
        currentMonster = monster; //현재 몬스터로 설정


        if (!isPlayerTurn)
            Renderer::DisplayUI(UIPart::CenterLeft, 2, monster.GetName() + "에게 기습당했습니다!!!");
    }


}

/// <summary>
/// 두 캐릭터를 넣으면 전투를 진행하는 함수입니다. 전투가 끝날 때까지 반복됩니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
void BattleManager::Battle(Player* player)
{
    if (isPlayerTurn)
        PlayerTurn(player, currentMonster);
    else
        MonsterTurn(player, currentMonster);
    isPlayerTurn = !isPlayerTurn;
}


#pragma endregion


#pragma region P&M Turn

/// <summary>
/// 플레이어 의 차례에 작동하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
void BattleManager::PlayerTurn(Player* player, Monster& monster)
{
    Renderer::DisplayUI(UIPart::CenterLeft, 3, player->GetName() + "의 턴!");
    std::uniform_int_distribution<int> dist(0, 99); //0~ 99의 균등 분포 생성

    if (isNIMO)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 3, "!!!!!귀여운 강아지를 처치하셧습니다!!!!!!");
        monster.SetHp(0);
        BattleEnd(player);
        return;
    }

    bool isplayerUseItem = dist(rng) < 20; // 20% 확률로 아이템 사용

    if (player->GetHp() < player->GetMaxHp() / 2 && !isplayerUseItem) // 체력이 절반 이하일 때 아이템 사용 확률 증가
    {
        isplayerUseItem = dist(rng) < 50; // 50% 확률로 아이템 사용, 더블체크로 체력이 낮을 때 아이템 사용 확률 증가
    }

    if (isplayerUseItem)
    {
        if (player->GetHp() == player->GetMaxHp()) // 최대 체력이면
        {
            player->GetInventory().UseItem(player, "공격력 포션", 1); //무조건 공격력 포션 사용
            useItemName = "공격력 포션";
        }
        else if (player->GetHp() < player->GetMaxHp() / 2) // 반피 이하면
        {
            player->GetInventory().UseItem(player, "HP 포션", 1);//반피 이하면 무조건 회복 포션
            useItemName = "HP 포션";
        }
        else // 그 사이 (반피 ~ 최대 미만)
        {
            int UseItemType = dist(rng) < 50 ? 0 : 1; // 50% 확률로 공격력 또는 회복 포션
            if (UseItemType == 0)
            {
                player->GetInventory().UseItem(player, "공격력 포션", 1); //공격력 포션 사용
                useItemName = "공격력 포션";

            }
            else
            {
                player->GetInventory().UseItem(player, "HP 포션", 1);
                useItemName = "HP 포션";

            }
        }

        Renderer::DisplayUI(UIPart::CenterLeft, 4, player->GetName() + "이" + useItemName + "을 사용했습니다!");

        return; // 아이템 사용 후 공격하지 않고 턴 종료
    }



    int PAttackDamage = player->GetAttack(); //플레이어 공격력 저장
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
void BattleManager::MonsterTurn(Player* player, Monster& monster)
{

    if (isNIMO)
    {
        if (isNIMO)
        {
            Renderer::DisplayUI(UIPart::CenterLeft, 3, "!!!니모의 튀어오르기!!!");
            Renderer::DisplayUI(UIPart::CenterLeft, 4, "그러나 아무 일도 일어나지 않았다!");
            return;
        }
    }
    Renderer::DisplayUI(UIPart::CenterLeft, 3, monster.GetName() + "의 턴!");

    std::uniform_int_distribution<int> dist(0, 99); //0~ 99의 균등 분포 생성

    int MAttackDamage = monster.GetAttack(); // 몬스터 공격력 저장
    if (dist(rng) < 10) // 10% 확률로 크리티컬 히트
    {
        MAttackDamage *= 2;
        Renderer::DisplayUI(UIPart::CenterLeft, 4, "크리티컬 히트!!!");
    }

    //플레이어 체력 변경
    player->SetHp(player->GetHp() - MAttackDamage);
    Renderer::DisplayUI(UIPart::CenterLeft, 5, player->GetName() + "의 체력이" + std::to_string(MAttackDamage) + "만큼 달았습니다!");

    //플레이어 사망 여부 확인
    if (player->GetHp() <= 0)
    {
        player->SetHp(0);
        Renderer::DisplayUI(UIPart::CenterLeft, 6, player->GetName() + "가 쓰러졌습니다!");
    }
    else //안죽었다면
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 6, player->GetName() + "의 남은 체력" + std::to_string(player->GetHp()));
    }

}

#pragma endregion


#pragma region BattleEnd


/// <summary>
/// 배틀종료 후 결과 처리 함수입니다.
/// </summary>
/// <param name="player">보상을 받을 플레이어</param>
/// <param name="monster">처치한 몬스터(돈/아이템 제공)</param>
void BattleManager::BattleEnd(Player* player)
{

    KillCount[currentMonster.GetName()]++;

    //연속전투 막기
    CurrentBattleState = EBattleState::Locked;

    //대래곤 처치시 엔딩
    if (isBoss)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "대래래래래래래래래래래래곤을 쓰러뜨렸다!!!!!!!!!!!!!!!");
        Renderer::DisplayUI(UIPart::CenterLeft, 3, "쓰러뜨렸다!!!!!!!!!!!!!");
        Renderer::DisplayUI(UIPart::CenterLeft, 4, "십억을 받았습니다.");
        return;
    }

    player->SetAttack(OriginalPlayerAttack); //플레이어 공격력을 원래대로 돌려놓습니다.
    //몬스터의 소지금 강탈
    player->SetMoney(player->GetMoney() + currentMonster.GetMoney());


    player->GainExp(50); // 경험치 50 획득

    if (isNIMO)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "동물학대범 타이틀을 획득했다!!");
        NimoDefeated = true;
        return;
    }


    Item* item = currentMonster.DropItem(100); //80%확률로 아이템 드랍
    if (item != nullptr)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 6, item->GetName() + "을(를) 획득했다!");
        player->GetInventory().AddItem(item);
    }

}

#pragma endregion


#pragma region GETTER

/// <summary>
/// 한번 이상 처치한 모든 몬스터의 처치횟수를 반환하는 함수
/// </summary>
void BattleManager::GetAllKillCount()
{
    for (auto& pair : KillCount) {
        IPCManager::GetInstance().SendLog(pair.first + " :" + std::to_string(pair.second) + "회");

    }
}

/// <summary>
/// 몬스터name을 집어넣으면 KillCount 횟수를 반환하는 함수
/// </summary>
/// <param name="name">몬스터이름</param>
/// <returns>처치한횟수</returns>
int BattleManager::GetKillCount(std::string name)
{
    int count = KillCount[name];
    return count;
}

#pragma endregion

