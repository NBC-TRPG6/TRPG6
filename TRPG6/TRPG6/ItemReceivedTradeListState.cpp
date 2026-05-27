#include "ItemReceivedTradeListState.h"
#include "TradeManager.h"
#include "Renderer.h"
#include "GameManager.h"
#include "Player.h"
#include "ItemTradeState.h"
#include "NetworkManager.h"

void ItemReceivedTradeListState::Enter()
{
}

void ItemReceivedTradeListState::Update(int ch, std::string & lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::Top, 0, "=== [ 나에게 온 거래 제안 목록 ] ===");
    // 1. 나에게 온 '대기 중(0)'인 거래들만 가져와서 저장
    currentDisplayedTrades = TradeManager::GetInstance().GetReceivedTrades(Client::playerName);

    if (currentDisplayedTrades.empty())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, "도착한 거래 제안이 없습니다.");
        Renderer::DisplayUI(UIPart::CenterLeft, 4, "0. 뒤로 가기");
    }
    else
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 2, " [번호] | [신청자] | [받을아이템] <- [줄아이템]");
        Renderer::DisplayUI(UIPart::CenterLeft, 3, "--------------------------------------------------");

        int line = 4;
        int index = 1;
        for (const auto& trade : currentDisplayedTrades)
        {
            std::string tradeLine = "  " + std::to_string(index++) + ".   | ["
                + std::string(trade.sender) + "] | "
                + std::string(trade.itemGiveName) + " <- "
                + std::string(trade.itemReceiveName);

            Renderer::DisplayUI(UIPart::CenterLeft, line++, tradeLine);
        }
        Renderer::DisplayUI(UIPart::CenterLeft, line + 1, "거래를 진행할 번호를 입력하세요.");
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 15, "0. 뒤로 가기");
    
    // 2. 번호 선택 처리 (거래 수락 로직)
    if (ch > 0 && ch <= (int)currentDisplayedTrades.size())
    {
        const TradeInfo & selectedTrade = currentDisplayedTrades[ch - 1];
        Player * myPlayer = GameManager::GetInstance().GetPlayer();
      
        // [체크] 나한테 줄 아이템(상대방이 요구한 아이템)이 있는지 확인
        auto* slot = myPlayer->GetInventory().GetItemSlot(selectedTrade.itemReceiveName);
      
        if (slot && slot->count >= selectedTrade.itemReceiveCount)
        {
            // A. 아이템이 있는 경우 -> 수락(1) 패킷 전송
            Pkt_TradeResponse resPkt;
            resPkt.tradeId = selectedTrade.tradeId;
            resPkt.response = 1; // Accepted
            
            NetworkManager::GetInstance().SendTradeResponse(resPkt);
            Renderer::DisplayUITimed(UIPart::CenterLeft, 18, "거래를 수락했습니다! 서버의 승인을 기다립니다...", 2.0f);
        }
        else
        {
            // B. 아이템이 없는 경우 -> 거래 실패 처리 (또는 자동 거절)
            Renderer::DisplayUITimed(UIPart::CenterLeft, 18, "실패: 요구하는 아이템(" +
                    std::string(selectedTrade.itemReceiveName) + ")이 부족합니다!", 2.0f);

            // 필요 시 자동으로 거절(2) 패킷을 보낼 수도 있습니다.
            Pkt_TradeResponse resPkt;
            resPkt.tradeId = selectedTrade.tradeId;
            resPkt.response = 2; // Declined
            NetworkManager::GetInstance().SendTradeResponse(resPkt);
        }
    }

    // 0번 입력 시 뒤로 가기
    if (ch == 0 || lastCommand == "0")
    {
        GameManager::GetInstance().SetCurrentState(new ItemTradeState());
    }
}
