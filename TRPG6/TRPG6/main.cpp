// 공식 라이브러리 ==========================================
#include <iostream>
#include <thread>
#include <vector>
#include <string>

// 우리들만의~ 라이브러리 ====================================
#include "NetworkManager.h" // 멀티 플레이 관리 반드시 최우선으로 임포트 할 것!!!!!!!!!!!!!!!
#include "Utils.h" // 윈도우 API 유틸리티
#include "DATABASE.h" // 전역 참조하는 헤더 데이터 파일
#include "Renderer.h" // 게임 렌더링
#include "Controller.h" // 입력 처리
#include "GameManager.h" // 게임 상태 관리
#include "IPCManager.h" // 다중 프롬프트 관리
#include "Player.h"
#include "BattleManager.h"
#include "Shop.h"
// 게임상태 ================================================
#include "IGameState.h"
#include "GameCreateState.h"

int main(int argc, char* argv[])
{
#pragma region LOGGING
    // 1. 자식 프로세스 분기
    if (argc > 1)
    {
        std::string arg = argv[1];
        if (arg == "--logviewer")
        {
            IPCManager::GetInstance().RunLogViewer();
            return 0;
        }
        else if (arg == "--multiviewer")
        {
            IPCManager::GetInstance().RunMultiViewer();
            return 0;
        }
    }

    // 2. 부모 프로세스(메인 게임) 초기화 및 자식 창 생성
    IPCManager::GetInstance().InitParentWindows();
#pragma endregion

#pragma region INIT
    // 컨트롤러 생성
    Controller controller;
    GameManager::GetInstance().SetCurrentState(new GameCreateState());

    // 화면 세팅
    std::cout << "\033[2J";
    HideCursor();
    auto art = LoadImageAsASCII("..\\..\\Resources\\mashutan.png");
    Renderer::SetTopASCIIImage(art); // 여기서 이미지를 등록하고 내부 Height를 계산함
    Renderer::Init();

    // 멀티 플레이 세팅
    NetworkManager::GetInstance().Init();
#pragma endregion

#pragma region MAIN_LOOP
    IPCManager::GetInstance().SendLog("게임 시스템 초기화 완료.");
    while (GameManager::GetInstance().GetIsGameRunning())
    {
        auto frameStart = std::chrono::steady_clock::now();

        // 1. 네트워크 루프
        // ...

        // 2. 렌더링 루프
        Renderer::UpdateTimedUI();
        Renderer::Render();

        // 3. 입력처리
        controller.ProcessInput();

        int ch = -1;
        bool isChatCommand = false; // 채팅 처리 여부 플래그

        if (!lastCommand.empty())
        {
            if (lastCommand == "111")
            {
                Client::CHAT_MODE = !Client::CHAT_MODE;
                READ_MODE = Client::CHAT_MODE;
                isChatCommand = true;
            }
            else if (Client::CHAT_MODE)
            {
                NetworkManager::GetInstance().SendChatPacket(Client::playerName, lastCommand);
                isChatCommand = true;
            }
            else
            {
                try { ch = std::stoi(lastCommand); }
                catch (...) { ch = -1; }
            }
        }

        // 4. 게임 상태 업데이트
        IGameState* currentState = GameManager::GetInstance().GetCurrentState();
        if (currentState != nullptr)
        {
            // 채팅 명령어였다면 게임 로직이 반응하지 않도록 빈 문자열 전달
            std::string cmdForState = isChatCommand ? "" : lastCommand;
            currentState->Update(ch, cmdForState);
        }

        if (!lastCommand.empty())
        {
            lastCommand.clear();
        }

        // 5. 프레임 처리
        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = frameEnd - frameStart;
        if (elapsed < FRAME_DURATION)
        {
            std::this_thread::sleep_for(FRAME_DURATION - elapsed);
        }
    }
#pragma endregion

    // 게임 종료
    // 시스템 정상 종료 시 파이프 닫기 (자식 창의 ReadFile 루프가 종료됨)
    IPCManager::GetInstance().SendLog("게임 종료 시퀀스 진입...");
    IPCManager::GetInstance().SendPlayerLeave(Client::playerName);
    IPCManager::GetInstance().SendChat(Client::playerName, "게임에서 나갔습니다!");
    IPCManager::GetInstance().Shutdown();

    std::cout << "\033[2J\033[1;1 HExit Game" << std::endl;

    return 0;
}
