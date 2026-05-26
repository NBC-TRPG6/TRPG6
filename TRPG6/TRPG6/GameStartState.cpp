#include "GameManager.h"
#include "GameStartState.h"
#include "Renderer.h"
#include "Player.h"
#include "BattleManager.h"
#include "BattleState.h"
#include "Shop.h"
#include "ShopBuyState.h"
#include "ShopSellState.h"
#include "ShopState.h"
#include "ShopStockState.h"
#include "IPCManager.h"
#include "Utils.h" //아스키 아트를 위한 include

void GameStartState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    IPCManager::GetInstance().SendLog("게임 진입");
    
    GameManager::GetInstance().SetFps(30.f);

}

void GameStartState::Update(int ch, std::string& lastCommand) {
    BattleManager battle;
    Shop shop;
    auto art = LoadImageAsASCII("..\\..\\Resources\\Dungeon5.png");
    Renderer::SetTopASCIIImage(art);


    GameManager::GetInstance().GetPlayer()->PrintStatus();

    // 5. 메뉴 출력 + switch

    Renderer::DisplayUI(UIPart::Top, 0, "메인 화면");
    Renderer::DisplayUI(UIPart::CenterLeft, 10, "1. 던전 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 11, "2. 상점 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 12, "3. 인벤토리 확인");
    switch (ch) {
    case 1: {
        GameManager::GetInstance().SetCurrentState(new BattleState());
        Renderer::ClearAllCenterLeftUI();


        break;
    }
    case 2:
        GameManager::GetInstance().SetCurrentState(new ShopState());
        Renderer::ClearAllCenterLeftUI();
        shop.ShowStock();
        break;

    case 3:
        Player * player = GameManager::GetInstance().GetPlayer();
        auto& inventory = player->GetInventory();
        const auto& slots = inventory.GetSlots();
        int size = (int)slots.size();

        if (size == 0)
        {
            Renderer::DisplayUI(UIPart::CenterLeft, 3, "인벤토리가 비어있습니다.");
        }
        else
        {
            for (int i = 0; i < size; ++i)
            {
                std::string name = slots[i].item->GetName();
                int count = slots[i].count;
                int sellPrice = slots[i].item->GetSellPrice();
                std::string info = std::to_string(i + 1) + ". " + name + " (x" + std::to_string(count) + ") | 가격: " + std::to_string(sellPrice) + "G";
                Renderer::DisplayUI(UIPart::CenterLeft, i + 3, info);
            }
        }
        break;


    }
}
