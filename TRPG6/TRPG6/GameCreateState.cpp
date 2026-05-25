#include "GameCreateState.h"
#include "GameLobbyState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "NetworkManager.h"
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
        if (ch == 1) // 1. 방 만들기 (SERVER)
        {
            Client::isServer = true;
            // 지정된 포트(예: 7777)로 서버 오픈 시도
            if (NetworkManager::GetInstance().StartHost(7777))
            {
                GameManager::GetInstance().SetCurrentState(new GameLobbyState());
            }
            else
            {
                IPCManager::GetInstance().SendLog("[오류] 방 만들기에 실패했습니다.");
            }
        }
        else if (ch == 2)
        {
            Client::isServer = false;
            createPhase = 2;
        }
    }
    else if (createPhase == 2) // 2. IP 입력 창
    {
        if (lastCommand == "0")
        {
            createPhase = 1;
            return;
        }

        std::string targetIP = lastCommand;

        // [방어 코드 1] IP 문자열 내부의 모든 공백, \n, \r 제거
        targetIP.erase(std::remove(targetIP.begin(), targetIP.end(), '\n'), targetIP.end());
        targetIP.erase(std::remove(targetIP.begin(), targetIP.end(), '\r'), targetIP.end());
        targetIP.erase(std::remove(targetIP.begin(), targetIP.end(), ' '), targetIP.end());

        if (targetIP.empty()) return;

        // [방어 코드 2] 연결(블로킹) 시도 전, 게임 화면에 연결 중임을 먼저 알림
        Renderer::ForceDisplayUI(UIPart::CenterLeft, 6, " > [" + targetIP + "] 연결 시도 중... (최대 수 초 대기)");

        if (NetworkManager::GetInstance().ConnectToServer(targetIP, 7777))
        {
            GameManager::GetInstance().SetCurrentState(new GameLobbyState());
        }
        else
        {
            IPCManager::GetInstance().SendLog("[오류] 접속 실패. 하마치 IP와 방장의 접속 여부를 확인하세요.");

            // [방어 코드 3] 실패 시 로비로 가지 않고, 본 게임 화면에 3초간 붉은색 경고 타이머 출력
            Renderer::DisplayUITimed(UIPart::CenterLeft, 8, " \033[31m[연결 실패] 올바른 하마치 IP인지 확인해주세요.\033[0m", 3.0f);
        }
    }
}
