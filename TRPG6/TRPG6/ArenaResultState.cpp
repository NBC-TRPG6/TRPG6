#include "ArenaResultState.h"
#include "GameStartState.h"
#include "GameManager.h"
#include "Renderer.h"

void ArenaResultState::Enter() {
    Renderer::ClearAllCenterLeftUI();
}

void ArenaResultState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 결과");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 메인화면으로");

    if (ch == 1) {
        GameManager::GetInstance().SetCurrentState(new GameStartState());
    }
}

void ArenaResultState::Exit() {
}
