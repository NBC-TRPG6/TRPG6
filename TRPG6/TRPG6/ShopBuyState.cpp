#include "ShopBuyState.h"
#include "GameManager.h"
#include "ShopState.h"
#include "Renderer.h"
#include "Player.h"
#include "Shop.h"
#include "Utils.h"

void ShopBuyState::Enter()
{
    auto art = LoadImageAsASCIIColor("..\\..\\Resources\\nemo.png");
    Renderer::SetTopASCIIImage(art);
}

void ShopBuyState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "==== [ 아이템 구매 ] ====");

    Shop* shop = GameManager::GetInstance().GetShop();
    int size = shop->GetStockSize();
    
    for (int i = 0; i < size; ++i)
    {
        std::string name = shop->GetItemNameByIndex(i);
        int price = shop->GetItemPriceByIndex(i);
        int stock = shop->GetItemStockByIndex(i);
        std::string info = std::to_string(i + 1) + ". " + name + " | 가격: " + std::to_string(price) + "G | 재고: " + std::to_string(stock);
        Renderer::DisplayUI(UIPart::CenterLeft, i + 2, info);
    }

    int msgLine = size + 3;
    Renderer::DisplayUI(UIPart::CenterLeft, msgLine, "0. 이전으로 돌아가기");

    if (size > 0)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, msgLine + 2, "구매할 아이템의 번호를 입력하세요.");
    }

    static std::string buyFeedback = "";
    Renderer::DisplayUI(UIPart::CenterLeft, msgLine + 4, buyFeedback);
   
    if (ch == 0)
    {
        buyFeedback = "";
        GameManager::GetInstance().SetCurrentState(new ShopState());
        return;
    }
    else if (ch >= 1 && ch <= size)
    {
        std::string targetName = shop->GetItemNameByIndex(ch - 1);
        if (shop->BuyItem(player, targetName))
        {
            buyFeedback = "[구매 완료]";
        }
        else
        {
            buyFeedback = "[구매 실패] 골드나 재고가 부족합니다.";
        }

    }
    else if(ch != -1)
    {
        buyFeedback = "[구매 실패] 유효한 아이템 번호를 입력하세요.";
    }
}
