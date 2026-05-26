#include "BattleState.h"
#include "BossMonster.h"
#include "GameManager.h"
#include "ShopBranchState.h"
#include "ClearState.h"
#include "DieState.h"


#include "Utils.h"



void BattleState::Enter()
{

    TurnCount = 0;
    isInit = false;
    isBattle = false;

    battleManager.SetBattleState(EBattleState::Ready);
    battleManager.StartBattle(GameManager::GetInstance().GetPlayer());

    std::string monsterName = "..\\..\\Resources\\" + battleManager.GetCurrentMonster().GetImageName() + ".png";
    auto art = LoadImageAsASCII(monsterName.c_str());
    Renderer::SetTopASCIIImage(art);
}

void BattleState::Update(int ch, std::string& lastCommand)
{

    Renderer::ClearAllCenterLeftUI();
    // 예시: 상태 진입 후 처음 한 번만 실행되도록 플래그 처리

    if (BattleEnded && !isBattle)
    {
        if (battleManager.GetIsBoss())
            GameManager::GetInstance().SetCurrentState(new ClearState());
        else
            GameManager::GetInstance().SetCurrentState(new ShopBranchState());
        return;
    }

    if (isInit && !isBattle)
    {
        isBattle = true;
        GameManager::GetInstance().SetFps(0.333333f);
    }
    if (!isInit || player == nullptr)
    {
        player = GameManager::GetInstance().GetPlayer();
        isInit = true;
    }

    if (battleManager.GetCurrentMonster().IsDead())
    {
        battleManager.BattleEnd(player);
        player->PrintStatus();
        isBattle = false;
        BattleEnded = true;
        return;
    }
    else if(player->IsDead())
    {
        Renderer::ClearAllCenterLeftUI();
        
        player->PrintStatus();
        isBattle = false;
        BattleEnded = true;

        GameManager::GetInstance().SetCurrentState(new DieState());
        return;
    }

    TurnCount++;
    Renderer::DisplayUI(UIPart::CenterLeft, 2, std::to_string(TurnCount) + "턴");
    battleManager.Battle(player);
    player->PrintStatus();
}

void BattleState::Exit()
{
    isInit = false;
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "배틀에서 나왔습니다.");
    GameManager::GetInstance().SetFps(30.f);
}

