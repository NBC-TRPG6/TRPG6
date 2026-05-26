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
#include "BattleManager.h"
#include "Shop.h"
// 게임상태 ================================================
#include "IGameState.h"
#include "GameStartState.h"

int main() {
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

    // 최초 플레이어 생성
    std::string name;
    std::cout << "이름을 입력하세요: ";
    std::cin >> name;

    GameManager::GetInstance().SetPlayer(new Player(name));
    BattleManager battle;
    Shop shop;
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
        // 5. 메뉴 출력 + switch
        Renderer::DisplayUI(UIPart::CenterLeft, 10, "1. 던전 입장");
        Renderer::DisplayUI(UIPart::CenterLeft, 11, "2. 상점 입장");
        Renderer::DisplayUI(UIPart::CenterLeft, 12, "3. 인벤토리 확인");
        switch (ch) {
        case 1: {
            Renderer::ClearAllCenterLeftUI();
            battle.SetBattleState(EBattleState::Ready);
            battle.StartBattle(*GameManager::GetInstance().GetPlayer());
            break;
        }
        case 2:
            Renderer::ClearAllCenterLeftUI();
            shop.ShowStock();
            Renderer::DisplayUI(UIPart::CenterLeft, 5, "1. 구매  2. 판매  3. 나가기");
            break;
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
    std::cout << "\033[2J\033[1;1 HExit Game" << std::endl;

    return 0;
}
