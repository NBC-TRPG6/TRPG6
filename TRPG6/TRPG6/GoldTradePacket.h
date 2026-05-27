// GoldTradePacket.h
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include "Packet.h" // PacketHeader 및 PacketType 참조

namespace GoldTradePacketType {
    enum {
        PKT_C2S_GOLD_TRADE_REQ = 210, // 골드 거래 요청 (클라이언트 -> 서버)
        PKT_S2C_GOLD_TRADE_ACK = 211  // 골드 거래 결과 알림 (서버 -> 클라이언트)
    };
}

#pragma pack(push, 1)

// 1. 클라이언트가 서버에 골드 송금을 요청하는 패킷
struct Pkt_GoldTradeRequest {
    PacketHeader header;
    char receiverName[32]; // 골드를 받을 대상 유저 이름
    int32_t goldAmount;     // 보낼 골드 액수

    Pkt_GoldTradeRequest() {
        header.size = sizeof(Pkt_GoldTradeRequest);
        header.type = static_cast<PacketType>(GoldTradePacketType::PKT_C2S_GOLD_TRADE_REQ);
        std::memset(receiverName, 0, sizeof(receiverName));
        goldAmount = 0;
    }
};

// 2. 서버가 검증 후 결과를 처리하여 브로드캐스트하는 패킷
struct Pkt_GoldTradeAck {
    PacketHeader header;
    char senderName[32];   // 보낸 사람 이름
    char receiverName[32]; // 받은 사람 이름
    int32_t goldAmount;    // 거래된 골드 액수
    uint8_t isSuccess;     // 1: 성공, 0: 실패 (잔액 부족 등)

    Pkt_GoldTradeAck() {
        header.size = sizeof(Pkt_GoldTradeAck);
        header.type = static_cast<PacketType>(GoldTradePacketType::PKT_S2C_GOLD_TRADE_ACK);
        std::memset(senderName, 0, sizeof(senderName));
        std::memset(receiverName, 0, sizeof(receiverName));
        goldAmount = 0;
        isSuccess = 0;
    }
};

#pragma pack(pop)

// 클라이언트 UI 등에서 호출할 송금 요청 함수 선언
void SendGoldTradeRequest(const std::string& receiverName, int32_t amount);
