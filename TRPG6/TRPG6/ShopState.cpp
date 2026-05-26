#include "ShopState.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "ShopBuyState.h"
#include "ShopStockState.h"
#include "ShopSellState.h"
#include "Renderer.h"
#include "Utils.h"

void ShopState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\nemo.png");
    Renderer::SetTopASCIIImage(art);
}

void ShopState::Update(int ch, std::string& lastCommand)
{
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "==== [ 상점 메뉴 ] ====");
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "무엇을 하시겠습니까?");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "0. 메인 메뉴로 돌아가기");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "1. 상점 재고 보기");
    Renderer::DisplayUI(UIPart::CenterLeft, 5, "2. 아이템 구매하기");
    Renderer::DisplayUI(UIPart::CenterLeft, 6, "3. 아이템 판매하기");

    switch (ch)
    {
    case 0:
        GameManager::GetInstance().SetCurrentState(new GameStartState());
        break;
    case 1:
        GameManager::GetInstance().SetCurrentState(new ShopStockState());
        break;
    case 2:
        GameManager::GetInstance().SetCurrentState(new ShopBuyState());
        break;
    case 3:
        GameManager::GetInstance().SetCurrentState(new ShopSellState());
        break;
    default:
        break;
    }
}
