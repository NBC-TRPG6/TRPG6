#include "GameManager.h"
#include "GameStartState.h"
#include "NetworkManager.h"
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
#include "TradeState.h"
#include "Utils.h" //아스키 아트를 위한 include
#include "InventoryState.h" // 인벤토리
#include "ItemTradeState.h" 
#include "ArenaReadyState.h"

void GameStartState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    IPCManager::GetInstance().SendLog("게임 진입");
    auto art = LoadImageAsASCII("..\\..\\Resources\\DungeonDoor.png");
    Renderer::SetTopASCIIImage(art);
    
    GameManager::GetInstance().SetFps(30.f);

}

void GameStartState::Update(int ch, std::string& lastCommand) {
    BattleManager battle;
    Shop shop;


    GameManager::GetInstance().GetPlayer()->PrintStatus();

    Renderer::DisplayUI(UIPart::Top, 0, "메인 화면");
    Renderer::DisplayUI(UIPart::CenterLeft, 7, "1. 던전 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 8, "2. 상점 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "3. 인벤토리 확인");
    Renderer::DisplayUI(UIPart::CenterLeft, 10, "4. 킬로그 확인");
    Renderer::DisplayUI(UIPart::CenterLeft, 11, "5. 아레나 개최");
    Renderer::DisplayUI(UIPart::CenterLeft, 12, "6. 거래 센터");

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

    case 5:
        if (!Client::isServer)
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 0, "\033[1;31m아레나는 방장만 개최할 수 있습니다!\033[0m", 2.0f);
            break;
        }
        GameManager::GetInstance().SetCurrentState(new ArenaReadyState());
        NetworkManager::GetInstance().BroadcastChangeState(EGameState::ArenaReady);
        IPCManager::GetInstance().SendLog("\033[1;34m방장이 아레나를 개최했습니다.\033[0m");
        break;

    case 6:
        Renderer::ClearAllCenterLeftUI();
        GameManager::GetInstance().SetCurrentState(new TradeState());
        break;
    }
}
