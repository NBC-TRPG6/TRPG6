#include "ArenaReadyState.h"
#include "ArenaLobbyState.h"
#include "GameManager.h"
#include "Renderer.h"

void ArenaReadyState::Enter() {
    Renderer::ClearAllCenterLeftUI();
}

void ArenaReadyState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 준비 중");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 아레나 로비 입장");

    if (ch == 1) {
        GameManager::GetInstance().SetCurrentState(new ArenaLobbyState());
    }
}

void ArenaReadyState::Exit() {
}
