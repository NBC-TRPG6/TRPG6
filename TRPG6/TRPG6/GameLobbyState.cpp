#include "GameLobbyState.h"
#include "GameCreateState.h"
#include "GameStartState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "DATABASE.h"
#include "IPCManager.h"

void GameLobbyState::Enter()
{
    READ_MODE = false;
    Renderer::ClearAllCenterLeftUI();

    IPCManager::GetInstance().SendPlayerJoin(Client::isServer, Client::playerName);
    IPCManager::GetInstance().SendChat(Client::playerName, "로비에 입장했습니다.");
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
            // (추후 여기에 클라이언트들에게 게임 시작 패킷을 쏘는 로직 추가)
            GameManager::GetInstance().SetCurrentState(new GameStartState());
        }
        else if (ch == 0)
        {
            IPCManager::GetInstance().SendPlayerLeave(Client::playerName);
            // 1을 넘겨주어 이름 입력(Phase 0)을 생략하고 방 선택(Phase 1)으로 복귀
            GameManager::GetInstance().SetCurrentState(new GameCreateState(1));
        }
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 6, " 방장이 게임을 시작할 때까지 대기중입니다...");
        Renderer::DisplayUI(UIPart::CenterLeft, 7, " 0. 접속 끊기 (뒤로가기)");

        // 클라이언트는 스스로 시작할 수 없으며 뒤로가기(0)만 가능
        if (ch == 0)
        {
            IPCManager::GetInstance().SendPlayerLeave(Client::playerName);
            GameManager::GetInstance().SetCurrentState(new GameCreateState(1));
        }
    }
}
