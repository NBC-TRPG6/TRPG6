#include "BossBattleState.h"
#include "GameManager.h"
#include "BossMonster.h"


#include "Utils.h"


void BossBattleState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\dragon.png");
    Renderer::SetTopASCIIImage(art);
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "보스 배틀에 돌입합니다!");
}

void BossBattleState::Update(int ch, std::string& lastCommand)
{
    // 예시: 상태 진입 후 처음 한 번만 실행되도록 플래그 처리
    static bool isInit = false;
    
    if (!isInit)
    {
        player = GameManager::GetInstance().GetPlayer();
        bossMonster = new BossMonster(player->GetLevel());
        isInit = true;
    }
    if (bossMonster == nullptr)
        int e = 23;

    Renderer::DisplayUI(UIPart::CenterLeft, 2, "보스 몬스터가 나타났습니다! ");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, bossMonster->GetName() + "이(가)");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "당신을 노려보고 있습니다!");

    battleManager.Battle(*player, *bossMonster);

    battleManager.PlayerTurn(*player, *bossMonster); // 플레이어의 턴을 시작합니다.

    if (player->IsDead())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 7, "당신은 보스 몬스터에게 패배했습니다...");
        Renderer::DisplayUI(UIPart::CenterLeft, 8, "한끼 식사로군요");
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 7, "당신은 보스 몬스터를 물리쳤습니다! 축카함");
    }
}

