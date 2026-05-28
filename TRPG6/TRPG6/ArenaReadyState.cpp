#include "ArenaReadyState.h"
#include "ArenaLobbyState.h"
#include "ArenaBettingState.h"
#include "GameManager.h"
#include "Renderer.h"
#include "DATABASE.h"
#include "ArenaNetworkManager.h"
#include "NetworkManager.h"
#include "ArenaBattleManager.h"
#include "IPCManager.h"
#include "Utils.h"
#include "Player.h"


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
    Renderer::DisplayUI(UIPart::CenterLeft, 8, "1. 아레나 로비 입장");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "2. 아이템 베팅");

    GameManager::GetInstance().GetPlayer()->PrintStatus();

    if(Client::isServer)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 10, "3. 아레나 준비 취소");

        if (ch == 3)
        {
            ArenaNetworkManager::GetInstance().CancelArenaPreparation();
        }
    }

    if (ch == 1) {
        if (ArenaBattleManager::GetInstance().GetHasBet())
        {
            if (Client::isServer)
            {
                if (NetworkManager::GetInstance().GetConnectedClientCount() >= 1)
                {
                    GameManager::GetInstance().SetCurrentState(new ArenaLobbyState());
                }
                else
                {
                    Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "참여 인원이 부족합니다!", 2.0f);
                }
            }
            else
            {
                GameManager::GetInstance().SetCurrentState(new ArenaLobbyState());
            }
        }
        else if(!ArenaBattleManager::GetInstance().GetHasBet())
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
