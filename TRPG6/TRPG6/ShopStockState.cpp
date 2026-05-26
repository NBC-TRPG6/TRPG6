#include "ShopStockState.h"
#include "GameManager.h"
#include "ShopState.h"
#include "Renderer.h"
#include "Player.h"
#include "Shop.h"
#include "Utils.h"

void ShopStockState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\nemo.png");
    Renderer::SetTopASCIIImage(art);
}

void ShopStockState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "==== [ 상점 재고 목록 ] ====");

    GameManager::GetInstance().GetShop()->ShowStock();

    int stockSize = GameManager::GetInstance().GetShop()->GetStockSize();
    Renderer::DisplayUI(UIPart::CenterLeft, stockSize + 2, "0. 이전으로 돌아가기");

    if (ch == 0)
    {
        GameManager::GetInstance().SetCurrentState(new ShopState());
    }
}
