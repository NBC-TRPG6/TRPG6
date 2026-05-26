#include "ArenaLobbyState.h"
#include "ArenaBattleState.h"
#include "GameManager.h"
#include "Renderer.h"
#include "NetworkManager.h"
#include "Utils.h"

void ArenaLobbyState::Enter() {
    Renderer::ClearAllCenterLeftUI();
    NetworkManager::GetInstance().SendArenaReadyPacket();
    auto art = LoadImageAsASCII("..\\..\\Resources\\colosseum2.png");
    Renderer::SetTopASCIIImage(art);
}

void ArenaLobbyState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 로비");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 전투 시작");

    if (ch == 1 && Client::isServer) {
        GameManager::GetInstance().SetCurrentState(new ArenaBattleState());
        NetworkManager::GetInstance().BroadcastChangeState(EGameState::ArenaBattle);
    }
}

void ArenaLobbyState::Exit() {
}
