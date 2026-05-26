#include "ArenaReadyState.h"
#include "ArenaLobbyState.h"
#include "ArenaBettingState.h"
#include "GameManager.h"
#include "Renderer.h"
#include "Utils.h"


// 아레나 준비 상태입니다.
// 게임 시작 전에 각 플레이어는 아이템을 걸고 참가합니다.
// 아이템을 적게 건 플레이어는 패널티를 받습니다.
// 아레나에 소집되면 나가는 것을 불가능
void ArenaReadyState::Enter() {
    Renderer::ClearAllCenterLeftUI();

    auto art = LoadImageAsASCII("..\\..\\Resources\\ArenaReady.png");
    Renderer::SetTopASCIIImage(art);
}

void ArenaReadyState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 준비 중");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 아레나 로비 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 10, "2. 아이템 베팅");

    if (ch == 1) {
        if (hasBet)
        {
            GameManager::GetInstance().SetCurrentState(new ArenaLobbyState());
        }
        else
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "먼저 아이템을 베팅해야 합니다!", 2.0f);
        }
    }
    else if (ch == 2) {
        GameManager::GetInstance().SetCurrentState(new ArenaBettingState());
    }
}

void ArenaReadyState::Exit() {
}
