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
#include "InventoryState.h" // 인벤토리

void GameStartState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    IPCManager::GetInstance().SendLog("게임 진입");
    
    GameManager::GetInstance().SetFps(30.f);

}

void GameStartState::Update(int ch, std::string& lastCommand) {
    BattleManager battle;
    Shop shop;
    auto art = LoadImageAsASCII("..\\..\\Resources\\DungeonDoor.png");
    Renderer::SetTopASCIIImage(art);


    GameManager::GetInstance().GetPlayer()->PrintStatus();

    // 5. 메뉴 출력 + switch

    Renderer::DisplayUI(UIPart::Top, 0, "메인 화면");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 던전 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 10, "2. 상점 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 11, "3. 인벤토리 확인");
    Renderer::DisplayUI(UIPart::CenterLeft, 12, "4. 킬로그 확인");
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
        GameManager::GetInstance().SetCurrentState(new InventoryState());
        break;

    case 4:
        GameManager::GetInstance().GetBattleManager()->GetAllKillCount();
        break;
    }
}
