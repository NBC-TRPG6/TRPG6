#include "ItemTradeState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "ItemTradeRequestState.h"
#include "ItemSentTradeListState.h"
#include "ItemReceivedTradeListState.h"
#include "Utils.h"

void ItemTradeState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\Potion.png");
    Renderer::SetTopASCIIImage(art);
}

void ItemTradeState::Update(int ch, std::string & lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::Top, 0, "=== [ 아이템 거래 센터 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "1. 거래 신청하기 (신규 제안)");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "2. 내가 보낸 거래 목록 보기");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "3. 나에게 온 거래 목록 보기");
    Renderer::DisplayUI(UIPart::CenterLeft, 6, "0. 메인 화면으로 돌아가기");

    // 입력 처리
    switch (ch)
    {
        case 0:
            GameManager::GetInstance().SetCurrentState(new GameStartState());
            break;
        case 1:
            GameManager::GetInstance().SetCurrentState(new ItemTradeRequestState());
            break;
        case 2:
            GameManager::GetInstance().SetCurrentState(new ItemSentTradeListState());
            break;
        case 3:
            GameManager::GetInstance().SetCurrentState(new ItemReceivedTradeListState());
            break;
        default:
            break;
    }
}
