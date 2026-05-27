#pragma once
#include <vector>
#include <string>
#include <mutex>
#include "Packet.h"

class TradeManager
{
public:
    static TradeManager & GetInstance()
    {
        static TradeManager instance;
        return instance;
    }

    // [공용] 서버로부터 동기화 패킷을 받았을 때 목록 업데이트
    void SyncTrade(const TradeInfo & info);

    // [서버 전용] 새로운 거래 신청을 받았을 때 ID 부여 및 목록 추가
    void Server_HandleRequest(const TradeInfo & info);

    // [서버 전용] 거래 응답(수락/거절)을 받았을 때 최종 처리
    void Server_HandleResponse(uint32_t tradeId, uint8_t response);

    // [UI용] 목록 필터링
    std::vector<TradeInfo> GetSentTrades(const std::string & myName);
    std::vector<TradeInfo> GetReceivedTrades(const std::string & myName);
    TradeInfo * GetTradeById(uint32_t tradeId);

private:
    TradeManager() = default;

    // 실제 아이템 이동 로직 (SyncTrade 내부에서 조건 충족 시 호출)
    void ApplyRealItemTrade(const TradeInfo& info);

    std::vector<TradeInfo> tradeList; // 모든 거래 내역 (메모리 관리용)
    std::mutex tradeMutex;
    uint32_t nextTradeId = 1;         // 서버가 부여할 다음 ID
};
