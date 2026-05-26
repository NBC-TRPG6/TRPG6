#include "ArenaLobbyState.h"
#include "ArenaBattleState.h"
#include "GameManager.h"
#include "Renderer.h"
#include "NetworkManager.h"
#include "Player.h"
#include "DATABASE.h"
#include "Utils.h"

void ArenaLobbyState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    auto art = LoadImageAsASCII("..\\..\\Resources\\colosseum2.png");
    Renderer::SetTopASCIIImage(art);
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
        if (net.IsArenaSnapshotCollecting())
        {
            Renderer::DisplayUI(UIPart::CenterLeft, 2, "스냅샷 수집 중...");
        }

        Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 전투 시작");

        if (ch == 1)
        {
            Player* player = GameManager::GetInstance().GetPlayer();
            if (net.RequestAllArenaPlayerSnapshots(player))
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 11,
                    "전투 준비: 호스트 스냅샷 전송, 클라이언트에 요청 중...", 2.0f);
            }
            else
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 11,
                    "전투를 시작할 수 없습니다. (로비 미완료 또는 수집 중)", 2.0f);
            }
        }
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 9,
            "방장이 전투를 시작하면 스냅샷이 자동 전송됩니다.");
    }
}

void ArenaLobbyState::Exit()
{
}
