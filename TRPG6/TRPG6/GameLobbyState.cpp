#include "NetworkManager.h"
#include "GameLobbyState.h"
#include "GameCreateState.h"
#include "GameStartState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "DATABASE.h"
#include "IPCManager.h"
#include "Utils.h"

void GameLobbyState::Enter()
{
    READ_MODE = false;
    Renderer::ClearAllCenterLeftUI();

    IPCManager::GetInstance().SendPlayerJoin(Client::isServer, Client::playerName);
    IPCManager::GetInstance().SendChat(Client::playerName, "로비에 입장했습니다.");

    auto art = LoadImageAsASCII("..\\..\\Resources\\Lobby.jpg");
    Renderer::SetTopASCIIImage(art);
}

void GameLobbyState::Update(int ch, std::string& lastCommand)
{

    Renderer::DisplayUI(UIPart::Top, 0, "[ GAME LOBBY ]");

    std::string role = Client::isServer ? "HOST(SERVER)" : "GUEST(CLIENT)";
    Renderer::DisplayUI(UIPart::CenterLeft, 2, " 현재 모드: " + role);
    Renderer::DisplayUI(UIPart::CenterLeft, 3, " 플레이어: " + Client::playerName);

    // 권한에 따른 분기 처리
    if (Client::isServer)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 6, " 1. 게임 시작하기");
        Renderer::DisplayUI(UIPart::CenterLeft, 7, " 0. 방 나가기 (뒤로가기)");

        if (ch == 1)
        {
            IPCManager::GetInstance().SendChat(Client::playerName, "게임을 시작합니다!");
            GameManager::GetInstance().SetCurrentState(new GameStartState());
            NetworkManager::GetInstance().BroadcastChangeState(EGameState::Start);
        }
        else if (ch == 0)
        {
            IPCManager::GetInstance().SendPlayerLeave(Client::playerName);

            // [수정] 방 폭파 (소켓 및 스레드 완전 종료 후 재초기화)
            NetworkManager::GetInstance().Shutdown();
            NetworkManager::GetInstance().Init();

            GameManager::GetInstance().SetCurrentState(new GameCreateState(1));
        }
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 6, " 방장이 게임을 시작할 때까지 대기중입니다...");
        Renderer::DisplayUI(UIPart::CenterLeft, 7, " 0. 접속 끊기 (뒤로가기)");

        if (ch == 0)
        {
            IPCManager::GetInstance().SendPlayerLeave(Client::playerName);

            // [수정] 서버와 연결 끊기 (소켓 완전 종료 후 재초기화)
            NetworkManager::GetInstance().Shutdown();
            NetworkManager::GetInstance().Init();

            GameManager::GetInstance().SetCurrentState(new GameCreateState(1));
        }
    }
}
