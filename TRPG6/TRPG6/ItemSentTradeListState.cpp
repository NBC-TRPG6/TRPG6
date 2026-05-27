#include "ItemSentTradeListState.h"
#include "TradeManager.h"
#include "Renderer.h"
#include "GameManager.h"
#include "ItemTradeState.h"
#include "DATABASE.h"

void ItemSentTradeListState::Enter()
{
}

void ItemSentTradeListState::Update(int ch, std::string & lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::Top, 0, "=== [ 내가 보낸 거래 신청 목록 ] ===");

    // 1. TradeManager에서 내가 보낸 거래들만 가져옴
    auto sentTrades = TradeManager::GetInstance().GetSentTrades(Client::playerName);

    if (sentTrades.empty())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "보낸 거래 신청이 없습니다.");
        Renderer::DisplayUI(UIPart::CenterLeft, 4, "0. 뒤로 가기");
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, " [상대방] | [보낸아이템] -> [받을아이템] | [상태]");
        Renderer::DisplayUI(UIPart::CenterLeft, 3, "--------------------------------------------------");

        int line = 4;
        for (const auto& trade : sentTrades)
        {
            // 상태 숫자(0,1,2,3)를 한글 문자열로 변환
            std::string statusStr;
            switch (trade.status)
            {
            case 0: statusStr = "대기 중"; break;
            case 1: statusStr = "성공(완료)"; break;
            case 2: statusStr = "거절됨"; break;
            case 3: statusStr = "취소됨"; break;
            default: statusStr = "알 수 없음"; break;
            }

            // 한 줄에 거래 정보 출력
            std::string tradeLine = "[" + std::string(trade.receiver) + "] | "
                + std::string(trade.itemGiveName) + " -> "
                + std::string(trade.itemReceiveName) + " | "
                + statusStr;
            Renderer::DisplayUI(UIPart::CenterLeft, line++, tradeLine);
        }
        Renderer::DisplayUI(UIPart::CenterLeft, line + 1, "0. 뒤로 가기");
    }

    // 0번 입력 시 이전 화면(거래 메인)으로 이동
    if (ch == 0 || lastCommand == "0")
    {
        GameManager::GetInstance().SetCurrentState(new ItemTradeState());
    }
}
