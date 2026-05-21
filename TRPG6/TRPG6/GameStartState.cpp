#include "GameManager.h"
#include "GameStartState.h"
#include "Renderer.h"

void GameStartState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "Hello, World");
    Renderer::DisplayUI(UIPart::CenterLeft, 0, "1번 라인");
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "2번 라인");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "3번 라인");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "4번 라인");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "5번 라인");
    Renderer::DisplayUI(UIPart::CenterLeft, 5, "6번 라인");

    //// 상태 전환
    //switch (ch) {
    //case 1: GameManager::GetInstance().SetCurrentState(new BattleMapState()); break;
    //case 2: GameManager::GetInstance().SetCurrentState(new JobSelectionState()); break;
    //case 3: GameManager::GetInstance().SetCurrentState(new CharacterUpgradeState()); break;
    //case 4: GameManager::GetInstance().SetCurrentState(new InventoryState()); break;
    //case 5: GameManager::GetInstance().SetCurrentState(new AlchemyWorkshopState()); break;
    //case 6:
    //    GameManager::GetInstance().SetCurrentState(new GameDefeatState());
    //    break;
    //}
}
