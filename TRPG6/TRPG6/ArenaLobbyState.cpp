#include "ArenaLobbyState.h"
#include "ArenaBattleState.h"
#include "GameManager.h"
#include "Renderer.h"
#include "NetworkManager.h"
#include "Player.h"
#include "DATABASE.h"

void ArenaLobbyState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    NetworkManager::GetInstance().SendArenaLobbyArrivedPacket();
}

void ArenaLobbyState::Update(int ch, std::string& lastCommand)
{
    (void)lastCommand;

    NetworkManager& net = NetworkManager::GetInstance();
    const int arrived = net.GetArenaLobbyArrivedCount();
    const int expected = net.GetExpectedArenaPlayerCount();

    Renderer::DisplayUI(UIPart::Top, 0, "아레나 로비");
    Renderer::DisplayUI(UIPart::CenterLeft, 0,
        "로비 도착: " + std::to_string(arrived) + " / " + std::to_string(expected));

    if (net.HasAllPlayersInArenaLobby())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 1, "전원 로비 도착 완료");
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 1, "다른 플레이어 대기 중...");
    }

    if (Client::isServer)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 전투 시작");

        if (ch == 1)
        {
            if (!net.HasAllPlayersInArenaLobby())
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 11,
                    "아직 전원이 로비에 도착하지 않았습니다.", 2.0f);
                return;
            }

            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                net.SendArenaPlayerSnapshotPacket(player);
            }

            Renderer::DisplayUITimed(UIPart::CenterLeft, 11,
                "스냅샷 전송 중... 전원 수집 후 전투가 시작됩니다.", 2.0f);
        }
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 9, "방장의 전투 시작을 기다리는 중...");
        if (ch == 1)
        {
            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                net.SendArenaPlayerSnapshotPacket(player);
                Renderer::DisplayUITimed(UIPart::CenterLeft, 11, "전투 준비 스냅샷을 전송했습니다.", 2.0f);
            }
        }
    }
}

void ArenaLobbyState::Exit()
{
}
