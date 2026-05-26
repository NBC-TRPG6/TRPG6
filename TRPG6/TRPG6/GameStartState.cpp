#include "GameManager.h"
#include "GameStartState.h"
#include "Renderer.h"
#include "Player.h"
#include "BattleManager.h"
#include "BattleState.h"
#include "Shop.h"
#include "IPCManager.h"

void GameStartState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    IPCManager::GetInstance().SendLog("게임 진입");
    GameManager::GetInstance().SetFps(30.f);
}

void GameStartState::Update(int ch, std::string& lastCommand) {
    //BattleManager battle; 나중에 정의되는대로 가져오기
    //Shop shop;

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
        //GameManager::GetInstance().SetCurrentState(new ShopState()); 작명하는거 따라가기
        Renderer::ClearAllCenterLeftUI();
        //shop.ShowStock();
        Renderer::DisplayUI(UIPart::CenterLeft, 5, "1. 구매  2. 판매  3. 나가기");
        break;
    }

}











