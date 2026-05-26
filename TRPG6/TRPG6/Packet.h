// Packet.h
#pragma once
#include <cstdint>
#include <cstring>
#include "DATABASE.h"

enum class PacketType : uint16_t {
    PKT_C2S_JOIN = 1,   // 클라이언트 -> 서버 (나 접속했어 + 이름 보냄)
    PKT_S2C_JOIN_ACK,   // 서버 -> 클라이언트 (접속 허가 + 서버 이름 보냄)
    PKT_S2C_LEAVE,      // 서버 -> 클라이언트 (누군가 나갔음)
    PKT_S2C_CHANGE_STATE,      // 서버 -> 클라이언트 (게임 상태 변경)
    PKT_CHAT,
};

#pragma pack(push, 1)
// 모든 네트워크 패킷의 공통 헤더 (4바이트)
struct PacketHeader {
    uint16_t size;   // 패킷 전체 크기 (헤더 + 데이터)
    PacketType type; // 패킷 종류
};
#pragma pack(pop)

// 가입 요청 및 승인 공용 구조체 추가
#pragma pack(push, 1)
struct Pkt_Join {
    PacketHeader header;
    char name[32];

    Pkt_Join(PacketType type)
    {
        header.size = sizeof(Pkt_Join);
        header.type = type;
        std::memset(name, 0, sizeof(name));
    }
};
#pragma pack(pop)

// 채팅 전송용 패킷 구조체
struct Pkt_Chat {
    PacketHeader header;
    char sender[32];   // 발신자 이름
    char message[128]; // 채팅 내용

    Pkt_Chat()
    {
        header.size = sizeof(Pkt_Chat);
        header.type = PacketType::PKT_CHAT;
        std::memset(sender, 0, sizeof(sender));
        std::memset(message, 0, sizeof(message));
    }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Pkt_ChangeState {
    PacketHeader header;
    EGameState targetState; // 변경할 목적지 상태

    Pkt_ChangeState()
    {
        header.size = sizeof(Pkt_ChangeState);
        header.type = PacketType::PKT_S2C_CHANGE_STATE;
        targetState = EGameState::Start;
    }
};
#pragma pack(pop)
