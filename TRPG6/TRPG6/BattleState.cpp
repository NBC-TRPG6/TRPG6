#include "BattleState.h"
#include "BossMonster.h"
#include "GameManager.h"
#include "GameStartState.h"


#include "Utils.h"


void BattleState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\dragon.png");
    Renderer::SetTopASCIIImage(art);
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "보스 배틀에 돌입합니다!");
    TurnCount = 0;
    GameManager::GetInstance().SetFps(0.333333f);
    GameManager::GetInstance().SetPlayer(new Player(Client::playerName));
    battleManager.StartBattle(*GameManager::GetInstance().GetPlayer());
}

void BattleState::Update(int ch, std::string& lastCommand)
{
    // 예시: 상태 진입 후 처음 한 번만 실행되도록 플래그 처리
    static bool isInit = false;

    if (!isInit)
    {
        player = GameManager::GetInstance().GetPlayer();
        isInit = true;
    }

    if (player->IsDead() || battleManager.GetCurrentMonster().IsDead())
    {
        battleManager.BattleEnd(*player);
        GameManager::GetInstance().SetCurrentState(new GameStartState());
        return;
    }

    TurnCount++;
    Renderer::DisplayUI(UIPart::CenterLeft, 2, std::to_string(TurnCount) + "턴");
    battleManager.Battle(*player);


}

void BattleState::Exit()
{
        Renderer::DisplayUI(UIPart::CenterLeft, 1, "배틀에서 나왔습니다.");
        GameManager::GetInstance().SetFps(8.f);
}

