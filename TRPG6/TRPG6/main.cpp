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
#include "IPCLogger.h"
// 게임상태 ================================================
#include "IGameState.h"
#include "GameStartState.h"

int main(int argc, char*argv[]) {
#pragma region LOGGING
    // 1. 자식 프로세스(로그 뷰어) 분기 처리
    if (argc > 1 && std::string(argv[1]) == "--logviewer")
    {
        IPCLogger::GetInstance().RunChild();
        return 0;
    }

    // 2. 부모 프로세스(메인 게임) 초기화 및 자식 창 생성
    IPCLogger::GetInstance().InitParent();
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
    Renderer::DisplayLog("렌더러 및 게임 시스템 초기화 완료.");

    // 최초 플레이어 생성
    std::string name;
    std::cout << "이름을 입력하세요: ";
    std::cin >> name;

    GameManager::GetInstance().SetPlayer(new Player(name));    
#pragma endregion

#pragma region MAIN_LOOP
    while (GameManager::GetInstance().GetIsGameRunning()) {
        // 1. 프레임 초기화
        // UIManager::DisplayASCIIAnimation(); // 재미용 아스키아트
        auto frameStart = std::chrono::steady_clock::now();

        // 2. UI 렌더링
        Renderer::UpdateTimedUI();
        Renderer::Render();

        // 3. 유저 입력 처리
        controller.ProcessInput();

        // 4. 입력 파싱 (중복 코드 제거)
        int ch = -1;
        if (!lastCommand.empty()) {
            try {
                ch = std::stoi(lastCommand);
            }
            catch (const std::invalid_argument&) {
                ch = -1;
            }
            catch (const std::out_of_range&) {
                ch = -1;
            }
        }

        // 5. 상태 변화
		// 이곳에서 여러분들의 코드가 실제로 진행됩니다.
        IGameState* currentState = GameManager::GetInstance().GetCurrentState();
        if (currentState != nullptr) {
            currentState->Update(ch, lastCommand);
        }

        // 6. 명령어 클리어
        lastCommand.clear();

        // 7. 프레임 제약
        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = frameEnd - frameStart;
        if (elapsed < FRAME_DURATION) {
            std::this_thread::sleep_for(FRAME_DURATION - elapsed);
        }
    }
#pragma endregion

	// 게임 종료
    // 시스템 정상 종료 시 파이프 닫기 (자식 창의 ReadFile 루프가 종료됨)
    Renderer::DisplayLog("게임 종료 시퀀스 진입...");
    IPCLogger::GetInstance().Shutdown();

    std::cout << "\033[2J\033[1;1 HExit Game" << std::endl;

    return 0;
}
