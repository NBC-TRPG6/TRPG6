#include "BattleState.h"
#include "BossMonster.h"
#include "GameManager.h"
#include "ShopBranchState.h"
#include "ClearState.h"
#include "DieState.h"
#include "Utils.h"
#include <chrono>



void BattleState::Enter()
{

    TurnCount = 0;
    isInit = false;
    isBattle = false;
    battleManager = GameManager::GetInstance().GetBattleManager();
    battleManager->SetBattleState(EBattleState::Ready);
    battleManager->StartBattle(GameManager::GetInstance().GetPlayer());

    std::string monsterName = "..\\..\\Resources\\" + battleManager->GetCurrentMonster().GetImageName() + ".png";
    auto art = LoadImageAsASCII(monsterName.c_str());
    Renderer::SetTopASCIIImage(art);
}

void BattleState::Update(int ch, std::string& lastCommand)
{
    if (waiting)
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActionTime).count();

        if (duration >= 1)
        {
            waiting = false; 
        }
        else
        {
            return; 
        }
    }

    if (!waiting)
    {
        waiting = true;
        lastActionTime = std::chrono::steady_clock::now();
        Renderer::ClearAllCenterLeftUI();

        if (BattleEnded && !isBattle)
        {
            //if (battleManager->GetIsBoss())
            //    ChangeState(new ClearState());
            //else
            //    ChangeState(new ShopBranchState());

            ChangeState(new ShopBranchState());
            return;
        }

        if (!isInit || player == nullptr)
        {
            player = GameManager::GetInstance().GetPlayer();
            isInit = true;
        }

        if (battleManager->GetCurrentMonster().IsDead())
        {
            battleManager->BattleEnd(player);
            player->PrintStatus();
            isBattle = false;
            BattleEnded = true;
            return;
        }
        else if (player->IsDead())
        {
            Renderer::ClearAllCenterLeftUI();

            player->PrintStatus();
            isBattle = false;
            BattleEnded = true;

            ChangeState(new DieState());
            return;
        }

        TurnCount++;
        Renderer::DisplayUI(UIPart::CenterLeft, 2, std::to_string(TurnCount) + "턴");
        battleManager->Battle(player);
        player->PrintStatus();
    }


}

void BattleState::Exit()
{
    isInit = false;
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "배틀에서 나왔습니다.");
    GameManager::GetInstance().SetFps(30.f);
}

void BattleState::ChangeState(IGameState* newState)
{
    GameManager::GetInstance().SetCurrentState(newState);
    //static auto lastActionTime = std::chrono::steady_clock::now();
    //auto now = std::chrono::steady_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActionTime).count();
    //
    //if (duration >= 2)
    //{
    //    GameManager::GetInstance().SetCurrentState(newState);
    //}
}

