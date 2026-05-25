#include "GameCreateState.h"
#include "GameLobbyState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "DATABASE.h"
#include "IPCManager.h"
#include "Player.h"
#include <iostream>

void GameCreateState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
}

void GameCreateState::ShowTitle()
{
    Renderer::DisplayUI(UIPart::Top, 0, "\033[32m[ TEXTRPG 6 : THE MULTIVERSE ]\033[0m");
}

void GameCreateState::ShowMenu()
{
    Renderer::ClearAllCenterLeftUI();

    if (createPhase == 0)
    {
        READ_MODE = true;
        Renderer::DisplayUI(UIPart::CenterLeft, 2, " > 캐릭터의 이름을 입력해주세요.");
    }
    else if (createPhase == 1)
    {
        READ_MODE = false;
        Renderer::DisplayUI(UIPart::CenterLeft, 2, " [ 접속 방식 선택 ]");
        Renderer::DisplayUI(UIPart::CenterLeft, 4, " 1. 방 만들기 (HOST)");
        Renderer::DisplayUI(UIPart::CenterLeft, 5, " 2. 방 참가하기 (CLIENT)");
    }
    else if (createPhase == 2)
    {
        READ_MODE = true;
        Renderer::DisplayUI(UIPart::CenterLeft, 2, " > 참가할 서버의 IP를 입력해주세요.");
        Renderer::DisplayUI(UIPart::CenterLeft, 4, " (취소하고 돌아가려면 '0' 입력)");
    }
}

void GameCreateState::Update(int ch, std::string& lastCommand)
{
    ShowTitle();
    ShowMenu();

    if (lastCommand.empty()) return;

    if (createPhase == 0)
    {
        Client::playerName = lastCommand;
        GameManager::GetInstance().SetPlayer(new Player(Client::playerName));
        createPhase = 1;
    }
    else if (createPhase == 1)
    {
        if (ch == 1)
        {
            Client::isServer = true;
            IPCManager::GetInstance().SendLog("서버 모드로 설정을 시작합니다.");
            GameManager::GetInstance().SetCurrentState(new GameLobbyState());
        }
        else if (ch == 2)
        {
            Client::isServer = false;
            createPhase = 2;
        }
    }
    else if (createPhase == 2)
    {
        // 0 입력 시 뒤로 가기 (이름 입력은 건너뛰고 방 선택 창으로)
        if (lastCommand == "0")
        {
            createPhase = 1;
            return;
        }

        std::string targetIP = lastCommand;
        IPCManager::GetInstance().SendLog("서버(" + targetIP + ")에 접속을 시도합니다.");
        GameManager::GetInstance().SetCurrentState(new GameLobbyState());
    }
}
