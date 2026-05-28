#include "TradeState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "ItemTradeState.h" // 아이템 담당 동료분의 State 헤더
#include "GoldTradeState.h" // 내가 만든 골드 전송 시스템 헤더

void TradeState::Enter()
{
    // 거래 센터에 진입할 때 딱 1번 화면을 깨끗하게 청소합니다.
    Renderer::ClearAllCenterLeftUI();
}

void TradeState::Update(int ch, std::string& lastCommand)
{
    // 1. 매 프레임 캐릭터 정보(우측 잔액창 등) 실시간 출력
    player = GameManager::GetInstance().GetPlayer();
    if (player != nullptr)
    {
        player->PrintStatus();
    }

    // 2. 상단 타이틀 설정
    Renderer::DisplayUI(UIPart::Top, 0, "=== 거래소 ===");

    // 3. 좌측 UI 출력 (12줄 제한 공간 내에서 딱 4줄만 사용하여 쾌적함)
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "   == [ 거래소 ] == ");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "   1. 아이템 거래 센터 이동");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "   2. 골드 전송 시스템 이동");
    Renderer::DisplayUI(UIPart::CenterLeft, 6, "   0. 메인 화면으로 돌아가기");

    // 4. 하위 선택 입력 처리 (ch 기반으로 오작동 없이 즉시 반응)
    switch (ch)
    {
    case 1:
        // 아이템 거래 센터로 이동 (이동 전 UI 청소)
        Renderer::ClearAllCenterLeftUI();
        GameManager::GetInstance().SetCurrentState(new ItemTradeState());
        break;

    case 2:
        // 내가 만든 골드 전송 시스템으로 이동 (이동 전 UI 청소)
        Renderer::ClearAllCenterLeftUI();
        GameManager::GetInstance().SetCurrentState(new GoldTradeState());
        break;

    case 0:
        // 다시 게임 메인 화면으로 복귀 (이동 전 UI 청소)
        Renderer::ClearAllCenterLeftUI();
        GameManager::GetInstance().SetCurrentState(new GameStartState());
        break;

    default:
        break;
    }
}
