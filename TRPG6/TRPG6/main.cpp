// 공식 라이브러리 ==========================================
#include <iostream>
#include <thread>
#include <vector>
#include <string>

// 우리들만의~ 라이브러리 ====================================
#include "Utils.h" // 윈도우 API 유틸리티
#include "DATABASE.h" // 전역 참조하는 헤더 데이터 파일
#include "Renderer.h" // 게임 렌더링
#include "Controller.h" // 입력 처리
#include "GameManager.h" // 게임 상태 관리
#include "Player.h"
#include "IPCManager.h"
// 게임상태 ================================================
#include "IGameState.h"
#include "GameStartState.h"

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
    GameManager::GetInstance().SetCurrentState(new GameStartState());

    // 화면 세팅
    std::cout << "\033[2J";
    HideCursor();
    auto art = LoadImageAsASCII("..\\..\\Resources\\mashutan.png");
    Renderer::SetTopASCIIImage(art); // 여기서 이미지를 등록하고 내부 Height를 계산함
    Renderer::Init();
#pragma endregion

#pragma region PLAYER_INIT
    // 최초 플레이어 생성
    std::string name;
    std::cout << "이름을 입력하세요: ";
    std::cin >> name;
    Client::playerName = name;
    GameManager::GetInstance().SetPlayer(new Player(Client::playerName));
#pragma endregion

#pragma region multi
    // 멀티 플레이 세팅
    IPCManager::GetInstance().SendLog("렌더러 및 게임 시스템 초기화 완료.");
    IPCManager::GetInstance().SendPlayerJoin(false, Client::playerName); // 일단 false로 고정, 나중에 Host/Client 구분해서 보내도록 수정 필요
    IPCManager::GetInstance().SendChat(Client::playerName, "게임에 접속했습니다!");
#pragma endregion    

#pragma region MAIN_LOOP
    while (GameManager::GetInstance().GetIsGameRunning())
    {
        // 1. 프레임 초기화
        // UIManager::DisplayASCIIAnimation(); // 재미용 아스키아트
        auto frameStart = std::chrono::steady_clock::now();

        // 2. UI 렌더링
        Renderer::UpdateTimedUI();
        Renderer::Render();

        // 3. 유저 입력 처리
        controller.ProcessInput();

        // 4. 입력 파싱 분기 처리 (채팅 vs 게임 명령어)
        int ch = -1;
        if (!lastCommand.empty())
        {
            if (lastCommand == "111")
            {
                Client::CHAT_MODE = !Client::CHAT_MODE;
                READ_MODE = Client::CHAT_MODE; // 문자열 입력 허용/차단 동기화
            }
            else if (Client::CHAT_MODE)
            {
                IPCManager::GetInstance().SendChat(Client::playerName, lastCommand);
            }
            else
            {
                try
                {
                    ch = std::stoi(lastCommand);
                }
                catch (const std::invalid_argument&)
                {
                    ch = -1;
                }
                catch (const std::out_of_range&)
                {
                    ch = -1;
                }

                // 5. 상태 변화 (게임 로직 진행)
                IGameState* currentState = GameManager::GetInstance().GetCurrentState();
                if (currentState != nullptr)
                {
                    currentState->Update(ch, lastCommand);
                }
            }

            // 6. 명령어 클리어 (어떤 처리를 했든 비워줌)
            lastCommand.clear();
        }

        // 7. 프레임 제약
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
