#include "ShopSellState.h"
#include "GameManager.h"
#include "ShopState.h"
#include "Renderer.h"
#include "Player.h"
#include "Shop.h"
#include "Utils.h"

void ShopSellState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\nemo.png");
    Renderer::SetTopASCIIImage(art);
}

void ShopSellState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();

    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "==== [ 아이템 판매 ] ====");

    auto& inventory = player->GetInventory();
    const auto& slots = inventory.GetSlots();
    int size = (int)slots.size();

    if (size == 0)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "판매 가능한 아이템이 없습니다.");
    }
    else
    {
        for (int i = 0; i < size; ++i)
        {
            std::string name = slots[i].item->GetName();
            int count = slots[i].count;
            int sellPrice = slots[i].item->GetSellPrice();
            std::string info = std::to_string(i + 1) + ". " + name + " (x" + std::to_string(count) + ") | 가격: " + std::to_string(sellPrice) + "G";
            Renderer::DisplayUI(UIPart::CenterLeft, i + 2, info);
        }
    }

    int menuStartLine = size + 4;
    Renderer::DisplayUI(UIPart::CenterLeft, menuStartLine, "0. 이전으로 돌아가기");

    if (size > 0)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, menuStartLine + 2, "판매할 아이템 번호를 입력하세요.");
    }

    static std::string sellFeedback = "";
    Renderer::DisplayUI(UIPart::CenterLeft, menuStartLine + 4, sellFeedback);

    if (ch == 0)
    {
        sellFeedback = "";
        GameManager::GetInstance().SetCurrentState(new ShopState());
        return;
    }
    else if (ch >= 1 && ch <= size)
    {
        std::string targetName = slots[ch - 1].item->GetName();
        int price = slots[ch - 1].item->GetSellPrice();
        if (GameManager::GetInstance().GetShop()->SellItem(player, targetName, 1))
        {
            sellFeedback = "[판매 완료] " + targetName + ": " + std::to_string(price) + "G";
        }
        else
        {
            sellFeedback = "[판매 실패] 아이템 정보를 확인해주세요.";
        }
    }
    else if (ch != -1)
    {
        sellFeedback = "[판매 실패] 유효한 아이템 번호를 입력하세요.";
    }
}
