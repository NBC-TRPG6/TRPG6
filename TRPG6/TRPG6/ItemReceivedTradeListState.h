#pragma once
#include "IGameState.h"
#include "Player.h"
#include "Packet.h"
#include <vector>

class ItemReceivedTradeListState : public IGameState
{
public:
    virtual void Enter() override;
    virtual void Update(int ch, std::string& lastCommand) override;

private:
    Player* player;
    // 현재 화면에 표시 중인 거래 목록을 임시 저장 (번호 선택용)
    std::vector<TradeInfo> currentDisplayedTrades;
};
