// Packet.h
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "DATABASE.h"

class Player;

enum class PacketType : uint16_t {
    // 접속용 패킷
    PKT_C2S_JOIN = 1,   // 클라이언트 -> 서버 (나 접속했어 + 이름 보냄)
    PKT_S2C_JOIN_ACK,   // 서버 -> 클라이언트 (접속 허가 + 서버 이름 보냄)
    PKT_S2C_LEAVE,      // 서버 -> 클라이언트 (누군가 나갔음)

    // 채팅용 패킷
    PKT_CHAT,

    // 브로드캐스팅용 패킷
    PKT_S2C_CHANGE_STATE,      // 서버 -> 클라이언트 (게임 상태 변경, EGameState)

    // 아레나 C2S (클라이언트 -> 서버)
    PKT_C2S_ARENA_ITEM_REGISTER,   // 보상 아이템 등록 (ArenaReadyState)
    PKT_C2S_ARENA_READY,           // 로비 도착 신고(ArenaLobby 진입 후 서버에 알림)
    PKT_C2S_ARENA_PLAYER_SNAPSHOT, // 전투 입장 스냅샷 (스탯 + 인벤, 가변 길이)
    PKT_C2S_ARENA_ATTACK,          // 공격 (targetName만, 공격자는 소켓 기준)
    PKT_C2S_ARENA_ITEM_USE,        // 아이템 사용

    // 아레나 S2C (서버 -> 클라이언트)
    PKT_S2C_ARENA_PLAYER_LIST,     // 전원 요약 스탯 UI
    PKT_S2C_ARENA_TURN_START,      // 현재 턴 플레이어
    PKT_S2C_ARENA_ATTACK_RESULT,   // 공격 결과 (서버 계산 damage)
    PKT_S2C_ARENA_HP_SYNC,         // HP 동기화 (전투 중 재스냅샷 없음, 방법 A)
    PKT_S2C_ARENA_ITEM_LIST,       // 턴 보유자 아이템 목록 (최대 MAX_ARENA_ITEM_SLOTS)
    PKT_S2C_ARENA_DIE,             // 사망 통지
    PKT_S2C_ARENA_RANK_LIST,       // 아레나 종료 순위
    PKT_S2C_ARENA_SNAPSHOT_REQUEST,  // 방장 전투 시작 시 게스트에게 스냅샷 전송 요청
    PKT_S2C_ARENA_SESSION_APPLY,     // 전투 종료 후 로컬 Player에 HP/인벤/보상 반영
    PKT_S2C_ARENA_LOBBY_STATE,       // 아레나 로비 상태

    // 아이템 거래
    PKT_C2S_TRADE_REQUEST,    // 거래 신청 (클라 -> 서버)
    PKT_C2S_TRADE_RESPONSE,        // 거래 수락/거절 (클라 -> 서버)
    PKT_S2C_TRADE_SYNC,            // 거래 목록 동기화 (서버 -> 클라)
};

#pragma pack(push, 1)

#pragma region Core Packet

// 모든 네트워크 패킷의 공통 헤더 (4바이트)
struct PacketHeader {
    uint16_t size;   // 패킷 전체 크기 (헤더 + 데이터)
    PacketType type; // 패킷 종류
};

// 가입 요청 및 승인 공용 구조체
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

#pragma endregion

#pragma region Arena Shared

// 인벤 슬롯 1칸 (스냅샷 / ItemList 공용)
struct ArenaItemSlot {
    char itemName[32];
    uint8_t itemType;  // ItemType enum 값
    int32_t value;     // 회복량 또는 버프 수치
    int32_t count;
};

// S2C PlayerList 한 줄
struct ArenaPlayerListEntry {
    char playerName[32];
    int32_t hp;
    int32_t maxHp;
    int32_t attack;
    int32_t level;
    uint8_t isAlive;   // 0 = 탈락, 1 = 생존
};

// S2C RankList 한 줄
struct ArenaRankEntry {
    char playerName[32];
    int32_t rank;
};

#pragma endregion

#pragma region Arena C2S

// Client_RewardItemRegistration
struct Pkt_ArenaItemRegister {
    PacketHeader header;
    char itemName[32];
    int32_t amount;

    Pkt_ArenaItemRegister()
    {
        header.size = sizeof(Pkt_ArenaItemRegister);
        header.type = PacketType::PKT_C2S_ARENA_ITEM_REGISTER;
        std::memset(itemName, 0, sizeof(itemName));
        amount = 0;
    }
};

struct Pkt_ArenaReady {
    PacketHeader header;

    Pkt_ArenaReady()
    {
        header.size = sizeof(Pkt_ArenaReady);
        header.type = PacketType::PKT_C2S_ARENA_READY;
    }
};

struct ArenaLobbyPlayerEntry {
    char playerName[32];
    uint8_t hasArrived;  // 0 = 대기, 1 = 로비 도착
};


// Client_PlayerObjectSend — 고정 헤더 뒤에 ArenaItemSlot[itemSlotCount] 가변 tail
// header.size = ArenaSnapshotPacketSize(itemSlotCount), itemSlotCount는 0~MAX_ARENA_ITEM_SLOTS
struct Pkt_ArenaPlayerSnapshotHeader {
    PacketHeader header;
    char playerName[32];
    int32_t maxHp;
    int32_t hp;
    int32_t attack;
    int32_t level;
    uint8_t itemSlotCount;
};

// Client_AttackRequest — Damage/AttackerName은 C2S에 넣지 않음
struct Pkt_ArenaAttack {
    PacketHeader header;
    char targetName[32];

    Pkt_ArenaAttack()
    {
        header.size = sizeof(Pkt_ArenaAttack);
        header.type = PacketType::PKT_C2S_ARENA_ATTACK;
        std::memset(targetName, 0, sizeof(targetName));
    }
};

struct Pkt_ArenaItemUse {
    PacketHeader header;
    char itemName[32];
    char targetName[32]; // 대상 없으면 빈 문자열(자기 자신)

    Pkt_ArenaItemUse()
    {
        header.size = sizeof(Pkt_ArenaItemUse);
        header.type = PacketType::PKT_C2S_ARENA_ITEM_USE;
        std::memset(itemName, 0, sizeof(itemName));
        std::memset(targetName, 0, sizeof(targetName));
    }
};

#pragma region Arena S2C


// 아레나 로비 상태
struct Pkt_ArenaLobbyState {
    PacketHeader header;
    uint8_t playerCount;
    ArenaLobbyPlayerEntry players[MAX_ARENA_PLAYERS];
    Pkt_ArenaLobbyState()
    {
        header.size = sizeof(Pkt_ArenaLobbyState);
        header.type = PacketType::PKT_S2C_ARENA_LOBBY_STATE;
        playerCount = 0;
        std::memset(players, 0, sizeof(players));
    }
};

// 방장 전투 시작 시 게스트에게 C2S 스냅샷 전송 요청
struct Pkt_ArenaSnapshotRequest {
    PacketHeader header;

    Pkt_ArenaSnapshotRequest()
    {
        header.size = sizeof(Pkt_ArenaSnapshotRequest);
        header.type = PacketType::PKT_S2C_ARENA_SNAPSHOT_REQUEST;
    }
};

struct Pkt_ArenaPlayerList {
    PacketHeader header;
    uint8_t playerCount;
    ArenaPlayerListEntry players[MAX_ARENA_PLAYERS];

    Pkt_ArenaPlayerList()
    {
        header.size = sizeof(Pkt_ArenaPlayerList);
        header.type = PacketType::PKT_S2C_ARENA_PLAYER_LIST;
        playerCount = 0;
        std::memset(players, 0, sizeof(players));
    }
};

struct Pkt_ArenaTurnStart {
    PacketHeader header;
    char turnPlayerName[32];

    Pkt_ArenaTurnStart()
    {
        header.size = sizeof(Pkt_ArenaTurnStart);
        header.type = PacketType::PKT_S2C_ARENA_TURN_START;
        std::memset(turnPlayerName, 0, sizeof(turnPlayerName));
    }
};

// Server_AttackResult
struct Pkt_ArenaAttackResult {
    PacketHeader header;
    char attackerName[32];
    char targetName[32];
    int32_t damage;

    Pkt_ArenaAttackResult()
    {
        header.size = sizeof(Pkt_ArenaAttackResult);
        header.type = PacketType::PKT_S2C_ARENA_ATTACK_RESULT;
        std::memset(attackerName, 0, sizeof(attackerName));
        std::memset(targetName, 0, sizeof(targetName));
        damage = 0;
    }
};

// Server_PlayerHpReturn — currentHp는 절대값(현재 체력)
struct Pkt_ArenaHpSync {
    PacketHeader header;
    char playerName[32];
    int32_t currentHp;
    int32_t maxHp;

    Pkt_ArenaHpSync()
    {
        header.size = sizeof(Pkt_ArenaHpSync);
        header.type = PacketType::PKT_S2C_ARENA_HP_SYNC;
        std::memset(playerName, 0, sizeof(playerName));
        currentHp = 0;
        maxHp = 0;
    }
};

struct Pkt_ArenaItemList {
    PacketHeader header;
    char ownerName[32];
    uint8_t slotCount;
    ArenaItemSlot slots[MAX_ARENA_ITEM_SLOTS];

    Pkt_ArenaItemList()
    {
        // 전송 전 slotCount 확정 시 header.size = offsetof(slots) + slotCount * sizeof(ArenaItemSlot) 로 갱신
        header.size = static_cast<uint16_t>(offsetof(Pkt_ArenaItemList, slots));
        header.type = PacketType::PKT_S2C_ARENA_ITEM_LIST;
        std::memset(ownerName, 0, sizeof(ownerName));
        slotCount = 0;
        std::memset(slots, 0, sizeof(slots));
    }
};

// Server_Die
struct Pkt_ArenaDie {
    PacketHeader header;
    char playerName[32];

    Pkt_ArenaDie()
    {
        header.size = sizeof(Pkt_ArenaDie);
        header.type = PacketType::PKT_S2C_ARENA_DIE;
        std::memset(playerName, 0, sizeof(playerName));
    }
};

// Server_ArenaRank — entryCount <= MAX_ARENA_PLAYERS
struct Pkt_ArenaRankList {
    PacketHeader header;
    uint8_t entryCount;
    ArenaRankEntry entries[MAX_ARENA_PLAYERS];

    Pkt_ArenaRankList()
    {
        header.size = sizeof(Pkt_ArenaRankList);
        header.type = PacketType::PKT_S2C_ARENA_RANK_LIST;
        entryCount = 0;
        std::memset(entries, 0, sizeof(entries));
    }
};

// 전투 종료 시 해당 클라이언트 Player에 반영 (가변: battleSlots + rewardSlots)
struct Pkt_ArenaSessionApplyHeader {
    PacketHeader header;
    int32_t hp;
    int32_t maxHp;
    int32_t attack;
    uint8_t battleSlotCount;
    uint8_t rewardSlotCount;
};


#pragma endregion
#pragma pack(pop)

#pragma region Item
#pragma pack(push, 1)
// 거래 하나에 대한 모든 정보를 담는 구조체
struct TradeInfo
{
    uint32_t tradeId;      // 방장(서버)이 부여하는 거래 고유 번호
    char sender[32];       // 거래를 신청한 사람 이름
    char receiver[32];     // 거래를 받을 사람 이름

    // [주는 아이템 정보]
    char itemGiveName[32];
    int itemGiveType;      // 0: HP_POTION, 1: ATTACK_BUFF, 2: MONSTER_PART
    int itemGiveValue;
    int itemGivePrice;
    int itemGiveCount;     // 몇 개 줄 것인지

    // [받고 싶은 아이템 정보]
    char itemReceiveName[32];
    int itemReceiveType;   // 0: HP_POTION, 1: ATTACK_BUFF, 2: MONSTER_PART
    int itemReceiveValue;
    int itemReceivePrice;
    int itemReceiveCount;  // 몇 개 받을 것인지

    // 거래 상태 (0: 대기 중, 1: 수락됨, 2: 거절됨, 3: 취소됨)
    uint8_t status;
};

// 거래 신청 패킷 (클라이언트가 서버로 보냄)
struct Pkt_TradeRequest
{
    PacketHeader header;
    TradeInfo info;

    Pkt_TradeRequest()
    {
        header.size = sizeof(Pkt_TradeRequest);
        header.type = PacketType::PKT_C2S_TRADE_REQUEST;
        std::memset(&info, 0, sizeof(info));
        info.status = 0; // 0: 대기 중 (Pending)
    }
};

// 거래 응답 패킷 (클라이언트가 서버로 보냄)
struct Pkt_TradeResponse
{
    PacketHeader header;
    uint32_t tradeId;      // 어떤 거래에 대한 응답인지
    uint8_t response;      // 1: 수락(Accepted), 2: 거절(Declined)

    Pkt_TradeResponse()
    {
        header.size = sizeof(Pkt_TradeResponse);
        header.type = PacketType::PKT_C2S_TRADE_RESPONSE;
        tradeId = 0;
        response = 2; // 기본값 거절
    }
};

// 거래 목록 동기화 패킷 (서버가 모든 클라이언트에게 보냄)
struct Pkt_TradeSync
{
    PacketHeader header;
    TradeInfo info; // 업데이트된 거래 정보

    Pkt_TradeSync()
    {
        header.size = sizeof(Pkt_TradeSync);
        header.type = PacketType::PKT_S2C_TRADE_SYNC;
        std::memset(&info, 0, sizeof(info));
    }
};
#pragma endregion

#pragma pack(pop)

#pragma region Arena Snapshot Helpers

// 가변 길이 스냅샷: Pkt_ArenaPlayerSnapshotHeader + itemSlotCount * ArenaItemSlot
inline constexpr size_t ArenaSnapshotHeaderSize()
{
    return sizeof(Pkt_ArenaPlayerSnapshotHeader);
}

inline size_t ArenaSnapshotPacketSize(uint8_t itemSlotCount)
{
    return ArenaSnapshotHeaderSize() + static_cast<size_t>(itemSlotCount) * sizeof(ArenaItemSlot);
}

inline bool IsValidArenaSnapshotSize(uint16_t packetSize, uint8_t itemSlotCount)
{
    if (itemSlotCount > MAX_ARENA_ITEM_SLOTS) return false;
    return packetSize == ArenaSnapshotPacketSize(itemSlotCount);
}

// 고정 길이 name 필드 복사 (버퍼 오버플로 방지)
inline void CopyStringToPacketField(char* dest, size_t destSize, const std::string& src)
{
    if (destSize == 0) return;
    std::memset(dest, 0, destSize);
    std::strncpy(dest, src.c_str(), destSize - 1);
}

// Player + 인벤(최대 MAX_ARENA_ITEM_SLOTS) -> PKT_C2S_ARENA_PLAYER_SNAPSHOT 버퍼
std::vector<char> BuildArenaPlayerSnapshotPacket(Player* player);

const ArenaItemSlot* GetArenaSnapshotItems(const Pkt_ArenaPlayerSnapshotHeader* snapshotHeader);

inline constexpr size_t ArenaSessionApplyHeaderSize()
{
    return sizeof(Pkt_ArenaSessionApplyHeader);
}

inline size_t ArenaSessionApplyPacketSize(uint8_t battleSlotCount, uint8_t rewardSlotCount)
{
    return ArenaSessionApplyHeaderSize()
        + static_cast<size_t>(battleSlotCount) * sizeof(ArenaItemSlot)
        + static_cast<size_t>(rewardSlotCount) * sizeof(ArenaItemSlot);
}

inline bool IsValidArenaSessionApplySize(uint16_t packetSize, uint8_t battleSlotCount, uint8_t rewardSlotCount)
{
    if (battleSlotCount > MAX_ARENA_ITEM_SLOTS || rewardSlotCount > MAX_ARENA_ITEM_SLOTS) return false;
    return packetSize == ArenaSessionApplyPacketSize(battleSlotCount, rewardSlotCount);
}

const ArenaItemSlot* GetArenaSessionApplyBattleSlots(const Pkt_ArenaSessionApplyHeader* hdr);
const ArenaItemSlot* GetArenaSessionApplyRewardSlots(const Pkt_ArenaSessionApplyHeader* hdr);

// S2C ArenaSessionApply 수신 후 로컬 Player HP/공격력/인벤(전투 소모+보상) 반영
void ApplyArenaSessionToLocalPlayer(Player* player, const char* packetData, size_t packetSize);

#pragma endregion
