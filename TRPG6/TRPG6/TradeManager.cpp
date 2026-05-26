#include "TradeManager.h"
#include "NetworkManager.h"
#include "GameManager.h"
#include "Player.h"
#include "Item.h"
#include "IPCManager.h"
#include <algorithm>

// [공용] 목록 동기화 (서버가 보낸 정보를 내 로컬 리스트에 반영)
void TradeManager::SyncTrade(const TradeInfo & info)
{
    std::lock_guard<std::mutex> lock(tradeMutex);

    // 이미 있는 거래면 업데이트, 없으면 추가
    auto it = std::find_if(tradeList.begin(), tradeList.end(), [&](const TradeInfo& t)
    {
        return t.tradeId == info.tradeId;
    });

    if (it != tradeList.end())
    {
        if (it->status == 0 && info.status == 1)
        {
            ApplyRealItemTrade(info); // 실제 아이템 이동 로직 호출
        }
        * it = info;
    }
    else
    {
        tradeList.push_back(info);
        // 만약 서버에서 처음부터 성공 상태로 보냈다면
        if (info.status == 1) ApplyRealItemTrade(info);
    }
}

// [서버 전용] 방장이 클라이언트의 거래 신청을 받았을 때
void TradeManager::Server_HandleRequest(const TradeInfo & info)
{
    if (!Client::isServer) return;

    TradeInfo newInfo = info;
    newInfo.tradeId = nextTradeId++; // 서버가 고유 ID 부여
    newInfo.status = 0; // Pending

    {
        std::lock_guard<std::mutex> lock(tradeMutex);
        tradeList.push_back(newInfo);
    }

    // 모든 플레이어에게 "새 거래 생겼다"고 전파
    Pkt_TradeSync syncPkt;
    syncPkt.info = newInfo;

    // NetworkManager를 통해 브로드캐스트 (이 함수는 나중에 구현)
    NetworkManager::GetInstance().BroadcastTradeSync(syncPkt);
    IPCManager::GetInstance().SendLog("[서버] 새로운 거래 신청 등록 (ID: " + std::to_string(newInfo.tradeId) + ")");
}

// [UI용] 내가 보낸 거래 목록 필터링
std::vector<TradeInfo> TradeManager::GetSentTrades(const std::string & myName)
{
    std::lock_guard<std::mutex> lock(tradeMutex);
    std::vector<TradeInfo> result;
    for (const auto& t : tradeList)
    {
        if (std::string(t.sender) == myName) result.push_back(t);
    }
    return result;
}

// [UI용] 나에게 온 거래 목록 필터링
std::vector<TradeInfo> TradeManager::GetReceivedTrades(const std::string & myName)
{
    std::lock_guard<std::mutex> lock(tradeMutex);
    std::vector<TradeInfo> result;
    for (const auto& t : tradeList)
    {
        // 나에게 왔고, 아직 대기 중(0)인 것만
        if (std::string(t.receiver) == myName && t.status == 0)
        {
            result.push_back(t);
        }
    }
    return result;
}

TradeInfo * TradeManager::GetTradeById(uint32_t tradeId)
{
    std::lock_guard<std::mutex> lock(tradeMutex);
    for (auto& t : tradeList)
    {
        if (t.tradeId == tradeId) return &t;
    }
    return nullptr;
}

void TradeManager::Server_HandleResponse(uint32_t tradeId, uint8_t response)
{
    if (!Client::isServer) return; // 방장만 실행

    TradeInfo * trade = GetTradeById(tradeId);
    if (!trade || trade->status != 0) return; // 이미 처리된 거래면 무시

    trade->status = response; // 1: 수락, 2: 거절

    if (response == 1)
    {
        // [수락됨] 아이템 이동 시작
        Pkt_TradeSync syncPkt;
        syncPkt.info = *trade;

        NetworkManager::GetInstance().BroadcastTradeSync(syncPkt);
        IPCManager::GetInstance().SendLog("[서버] 거래 성사! (ID: " + std::to_string(tradeId) + ")");
    }
    else
    {
        // [거절됨]
        Pkt_TradeSync syncPkt;
        syncPkt.info = *trade;
        NetworkManager::GetInstance().BroadcastTradeSync(syncPkt);
        IPCManager::GetInstance().SendLog("[서버] 거래 거절됨. (ID: " + std::to_string(tradeId) + ")");
    }
}

// 실제로 인벤토리를 건드리는 함수
void TradeManager::ApplyRealItemTrade(const TradeInfo & info)
{
    Player * myPlayer = GameManager::GetInstance().GetPlayer();
    if (!myPlayer) return;

    // 내가 신청자(A)인 경우: 내 아이템(Give)을 주고 상대 아이템(Receive)을 받음
    if (std::string(info.sender) == Client::playerName)
    {
        myPlayer->GetInventory().UseItem(nullptr, info.itemGiveName, info.itemGiveCount);

        // 아이템 생성 
        Item* newItem = new Item(info.itemReceiveName, (ItemType)info.itemReceiveType, info.itemReceiveValue, info.itemReceivePrice);
        myPlayer->GetInventory().AddItem(newItem, info.itemReceiveCount);

        IPCManager::GetInstance().SendLog("[거래] '" + std::string(info.receiver) + "'님과의 거래 성사! 아이템이 교환되었습니다.");
    }

    // 내가 수락자(B)인 경우: 내 아이템(Receive)을 주고 상대 아이템(Give)을 받음
    else if (std::string(info.receiver) == Client::playerName)
    {
        myPlayer->GetInventory().UseItem(nullptr, info.itemReceiveName, info.itemReceiveCount);

        Item * newItem = new Item(info.itemGiveName, static_cast<ItemType>(info.itemGiveType), info.itemGiveValue, info.itemGivePrice);
        myPlayer->GetInventory().AddItem(newItem, info.itemGiveCount);

        IPCManager::GetInstance().SendLog("[거래] '" + std::string(info.sender) + "'님과의 거래 성사! 아이템이 교환되었습니다.");
    }
}
