#include "GoldTradeState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "GoldTradeRequestState.h"

void GoldTradeState::Enter()
{
}

void GoldTradeState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "=== [ 골드 전송 시스템 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "1. 골드 보내기");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "0. 메인 화면으로 돌아가기");

    // 입력 처리
    switch (ch)
    {
    case 0:
        GameManager::GetInstance().SetCurrentState(new GameStartState());
        break;
    case 1:
        GameManager::GetInstance().SetCurrentState(new GoldTradeRequestState());
        break;
    default:
        break;
    }
}
