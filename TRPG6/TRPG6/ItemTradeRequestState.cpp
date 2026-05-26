#include "ItemTradeRequestState.h"
#include "Renderer.h"
#include "NetworkManager.h"
#include "TradeManager.h"
#include "GameManager.h"
#include "ItemTradeState.h"
#include "Shop.h"
#include <sstream>
#include <vector>

// 문자열을 구분자로 나누는 유틸리티 함수
std::vector<std::string> Split(const std::string & s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

void ItemTradeRequestState::Enter()
{}

void ItemTradeRequestState::Update(int ch, std::string & lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::Top, 0, "=== [ 거래 신청서 작성 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "형식: [받을사람]/[줄아이템이름]/[받을아이템이름]");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "0. 아이템 거래 센터로 이동");

    if (lastCommand == "0")
    {
        GameManager::GetInstance().SetCurrentState(new ItemTradeState());
        return;
    }
    else if (lastCommand.empty())
    {
        return;
    }

    std::vector<std::string> parts = Split(lastCommand, '/');

    if (parts.size() != 3)
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 10, "입력 형식이 잘못되었습니다! (받을사람/줄아이템/받을아이템)", 2.0f);
        return;
    }

    std::string receiverName = parts[0];
    std::string giveItemName = parts[1];
    std::string receiveItemName = parts[2];
 
    // 유효성 검사
    // (1) 줄 아이템이 내 인벤토리에 있는지 확인
    auto* giveSlot = player->GetInventory().GetItemSlot(giveItemName);
    if (!giveSlot || giveSlot->count <= 0)
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 10, "해당 아이템을 보유하고 있지 않습니다!", 2.0f);
        return;
    }

    // (2) 받을 아이템이 게임 내에 존재하는지 확인 (간단한 예시: 특정 목록에 있는지)
    // 여기서는 간단하게 상점(Shop)에 있는 아이템인지 확인하는 로직을 예로 듭니다.
    // 만약 별도의 아이템 도감이 있다면 그곳을 확인해야 합니다.
    bool isValidItem = false;
    Shop* shop = GameManager::GetInstance().GetShop();
    for (int i = 0; i < shop->GetStockSize(); ++i)
    {
        if (shop->GetItemNameByIndex(i) == receiveItemName)
        {
            isValidItem = true;
            break;
        }
    }

    if (!isValidItem)
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 10, "존재하지 않는 아이템입니다!", 2.0f);
        return;
    }

    // 3. 모든 검사 통과 시 패킷 생성 및 전송
    Pkt_TradeRequest pkt;

    // 신청자 정보
    strcpy_s(pkt.info.sender, Client::playerName.c_str());
    strcpy_s(pkt.info.receiver, receiverName.c_str());

    // 주는 아이템 정보 (내 인벤토리 슬롯에서 가져옴)
    strcpy_s(pkt.info.itemGiveName, giveSlot->item->GetName().c_str());
    pkt.info.itemGiveType = static_cast<int>(giveSlot->item->GetType());
    pkt.info.itemGiveValue = giveSlot->item->GetValue();
    pkt.info.itemGivePrice = giveSlot->item->GetPrice();
    pkt.info.itemGiveCount = 1; // 기본 1개 (필요 시 확장 가능)

    // 받고 싶은 아이템 정보 (이름만 넣어서 보냄, 상세 정보는 나중에 서버나 상대방이 채움)
    strcpy_s(pkt.info.itemReceiveName, receiveItemName.c_str());
    pkt.info.itemReceiveCount = 1;

    // 패킷 전송
    NetworkManager::GetInstance().SendTradeRequest(pkt);

    Renderer::DisplayUITimed(UIPart::CenterLeft, 10, receiverName + "님에게 거래를 신청했습니다!", 2.0f);
}
