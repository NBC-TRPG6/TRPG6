#include "ArenaLobbyState.h"
#include "ArenaBattleState.h"
#include "GameManager.h"
#include "Renderer.h"

void ArenaLobbyState::Enter() {
    Renderer::ClearAllCenterLeftUI();
}

void ArenaLobbyState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 로비");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 전투 시작");

    if (ch == 1) {
        GameManager::GetInstance().SetCurrentState(new ArenaBattleState());
    }
}

void ArenaLobbyState::Exit() {
}
