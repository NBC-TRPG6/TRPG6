#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "Packet.h"
#include "Item.h"

class Player;

class ArenaNetworkManager {
public:
    static ArenaNetworkManager& GetInstance()
    {
        static ArenaNetworkManager instance;
        return instance;
    }

#pragma region Packet Dispatch

    bool TryHandlePacket(SOCKET sock, PacketHeader* header);
    // 호스트: 접속 해제 시 로비 명단 갱신
    void OnPlayerDisconnected();

#pragma endregion

#pragma region Lobby

    // ---------- 로비 UI (S2C PKT_S2C_ARENA_LOBBY_STATE 캐시) ----------

    // S2C 캐시: 예정 인원 전원이 [도착]인지(ArenaLobbyState 완료 판정)
    bool HasAllPlayersInArenaLobby() const;
    // S2C 캐시: 도착 인원 수(N in “N / M”)
    int GetArenaLobbyArrivedCount() const;
    // S2C 캐시: 참가 예정 인원 수(M in “N / M”, 방장+접속자)
    int GetExpectedArenaPlayerCount() const;
    // S2C 캐시: 참가자별 [도착]/[대기] 명단(ArenaLobbyState 목록 표시)
    const std::vector<ArenaLobbyPlayerEntry>& GetArenaLobbyPlayers() const;
    // 호스트: arenaLobbyArrived·arenaLobbyDisplay 초기화
    void ResetArenaLobbyArrivalTracking();

#pragma endregion

#pragma region Battle Flow

    // ---------- 호스트 전용 (C2S 수집 · S2C CHANGE_STATE 조합) ----------

    // 호스트: 전원 로비 도착 후 스냅샷 수집 시작(호스트 C2S + S2C SNAPSHOT_REQUEST)
    bool RequestAllArenaPlayerSnapshots(Player* hostPlayer);
    // 호스트: 스냅샷 수집 중 여부(ArenaLobbyState “수집 중…” 표시)
    bool IsArenaSnapshotCollecting() const { return arenaSnapshotCollecting; }

#pragma endregion

#pragma region S2C Broadcast

    // ---------- S2C 브로드캐스트 (호스트 → 접속자 전원 + 호스트 로컬 반영) ----------

    // S2C PKT_S2C_ARENA_PLAYER_LIST: 전투 참가자 HP·공격력 등 요약 목록
    void BroadcastArenaPlayerList(const Pkt_ArenaPlayerList& pkt);
    // S2C PKT_S2C_ARENA_TURN_START: 현재 턴 플레이어 이름
    void BroadcastArenaTurnStart(const std::string& turnPlayerName);
    // S2C PKT_S2C_ARENA_ATTACK_RESULT: 서버 계산 공격 결과(데미지·명중)
    void BroadcastArenaAttackResult(const Pkt_ArenaAttackResult& pkt);
    // S2C PKT_S2C_ARENA_HP_SYNC: HP 절대값 동기화(전투 중 재스냅샷 없음)
    void BroadcastArenaHpSync(const Pkt_ArenaHpSync& pkt);
    // S2C PKT_S2C_ARENA_ITEM_LIST: 턴 보유자가 쓸 수 있는 아이템 슬롯 목록
    void BroadcastArenaItemList(const Pkt_ArenaItemList& pkt);
    // S2C PKT_S2C_ARENA_DIE: 사망 통지
    void BroadcastArenaDie(const std::string& playerName);
    // S2C PKT_S2C_ARENA_RANK_LIST: 전투 종료 순위
    void BroadcastArenaRankList(const Pkt_ArenaRankList& pkt);
    // S2C PKT_S2C_ARENA_LOBBY_STATE: 로비 참가자·도착 여부 명단 갱신
    void BroadcastArenaLobbyState();
    // S2C PKT_S2C_ARENA_ITEM_RESULT: 사용한 아이템의 정보 갱신
    void BroadcastArenaItemResult(const Pkt_ArenaItemResult& pkt);
    // S2C 턴 시작 + 해당 턴 아이템 목록을 한 번에 브로드캐스트
    void BroadcastArenaTurnAndItems(const std::string& playerName);
    // S2C PKT_S2C_ARENA_REWARD_POOL: 보상 화면용 패킷(순위별 보상 아이템 목록)
    void BroadcastArenaRewardPool();
    // S2C 스냅샷 기준 PlayerList 재브로드캐스트(HP·버프 변경 후 UI 동기화)
    void BroadcastArenaPlayerListFromSnapshots();
    // S2C PKT_S2C_ARENA_SNAPSHOT_REQUEST: 게스트에게 스냅샷 C2S 전송 요청
    void BroadcastArenaSnapshotRequest();

#pragma endregion

#pragma region Session State

    // 호스트: 아레나 맵·플래그·로비 추적 일괄 초기화(Shutdown 등)
    void ClearSessionData();

#pragma endregion

#pragma region C2S Send

    // ---------- C2S (클라이언트 → 호스트) ----------

    // C2S PKT_C2S_ARENA_ITEM_REGISTER: 베팅 아이템 등록(ArenaReady / ArenaBetting)
    void SendArenaItemRegisterPacket(const std::string& itemName, int count,
        ItemType itemType, int32_t value);
    // 호스트: 아레나 준비 취소 — 베팅 반환 S2C + Start 동기화
    void CancelArenaPreparation();
    // C2S PKT_C2S_ARENA_READY: 로비 화면 진입 후 “도착” 신고(상태 전환 없음)
    void SendArenaLobbyArrivedPacket();
    // C2S PKT_C2S_ARENA_PLAYER_SNAPSHOT: 전투용 스탯·인벤 스냅샷 전송(가변 길이, 최대 14슬롯)
    void SendArenaPlayerSnapshotPacket(Player* player);
    // C2S PKT_C2S_ARENA_ATTACK: 공격 요청(공격자는 송신 소켓/호스트 이름으로 판별)
    void SendArenaAttackPacket(const std::string& targetName);
    // C2S PKT_C2S_ARENA_ITEM_USE: 아이템 사용(targetName 비우면 자신에게 사용)
    void SendArenaItemUsePacket(const std::string& itemName, const std::string& targetName);

#pragma endregion

private:
    ArenaNetworkManager() = default;

#pragma region Packet Builders

    // ---------- S2C (호스트 → 클라이언트) ----------

    // S2C PKT_S2C_ARENA_LOBBY_STATE: arenaLobbyArrived 기준으로 송신 패킷 조립
    void BuildArenaLobbyStatePacket(Pkt_ArenaLobbyState& out) const;
    // S2C PKT_S2C_ARENA_PLAYER_LIST: arenaPlayerSnapshots에서 목록 패킷 생성
    void BuildArenaPlayerListPacket(Pkt_ArenaPlayerList& out) const;
    // S2C PKT_S2C_ARENA_ITEM_LIST: 스냅샷 인벤에서 턴 보유자 아이템 목록 생성
    bool BuildArenaItemListPacket(const std::string& ownerName, Pkt_ArenaItemList& out) const;
    // S2C PKT_S2C_ARENA_SESSION_APPLY: 순위·보상 반영용 종료 패킷(플레이어별 가변)
    bool BuildArenaSessionApplyPacket(const std::string& playerName, const Pkt_ArenaRankList& rankPkt,
        std::vector<char>& outBuffer) const;
    // S2C PKT_S2C_ARENA_REWARD_POOL: arenaPlayerSnapshots + rankPkt에서 보상 풀 패킷 생성
    void BuildArenaRewardPoolPacket(Pkt_ArenaRewardPool& out) const;
    // 호스트: RankList에서 플레이어 순위 조회(1위 보상 등)
    static int GetArenaRankForPlayer(const Pkt_ArenaRankList& rankPkt, const std::string& playerName);

#pragma endregion

#pragma region Lobby

    // S2C PKT_S2C_ARENA_LOBBY_STATE: 수신 패킷을 arenaLobbyDisplay 캐시에 반영
    void ApplyArenaLobbyStateCache(const Pkt_ArenaLobbyState& pkt);
    // 호스트: 방장+clientNames 참가 예정 이름 집합(로비·스냅샷 “전원” 판정)
    std::set<std::string> CollectExpectedArenaPlayerNames() const;

#pragma endregion

#pragma region S2C Local Apply (Notify)

    // ---------- S2C 로컬 UI 푸시 (ArenaBattleState, 호스트·게스트 공통) ----------

    // S2C PKT_S2C_ARENA_PLAYER_LIST → ArenaBattleState 플레이어 목록 갱신
    void NotifyArenaBattlePlayerList(const Pkt_ArenaPlayerList& pkt);
    // S2C PKT_S2C_ARENA_TURN_START → 현재 턴 표시 갱신
    void NotifyArenaBattleTurnStart(const std::string& turnPlayerName);
    // S2C PKT_S2C_ARENA_ATTACK_RESULT → 공격 결과 로그·UI 갱신
    void NotifyArenaBattleAttackResult(const Pkt_ArenaAttackResult& pkt);
    // S2C PKT_S2C_ARENA_HP_SYNC → HP 바 갱신
    void NotifyArenaBattleHpSync(const Pkt_ArenaHpSync& pkt);
    // S2C PKT_S2C_ARENA_ITEM_LIST → 턴 보유자 아이템 메뉴 갱신
    void NotifyArenaBattleItemList(const Pkt_ArenaItemList& pkt);
    // S2C PKT_S2C_ARENA_DIE → 사망자 UI 처리
    void NotifyArenaBattleDie(const std::string& playerName);
    // S2C PKT_S2C_ARENA_ITEM_RESULT → 아이템 사용 결과 갱신
    void NotifyArenaItemResult(const Pkt_ArenaItemResult& pkt);

#pragma endregion

#pragma region Host C2S Receive

    // ---------- C2S 수신 처리 (호스트 ProcessPacket) ----------

    // C2S PKT_C2S_ARENA_ITEM_REGISTER: 베팅 풀에 아이템 등록
    void OnHostArenaItemRegister(SOCKET sock, const Pkt_ArenaItemRegister& pkt);
    // C2S PKT_C2S_ARENA_READY: 로비 도착 기록 후 S2C LOBBY_STATE 브로드캐스트
    void OnHostArenaLobbyArrived(SOCKET sock);
    // C2S PKT_C2S_ARENA_PLAYER_SNAPSHOT: 스냅샷 저장, 전원 수집 시 전투 시작
    void OnHostArenaPlayerSnapshot(SOCKET sock, const char* packetData, size_t packetSize);
    // C2S PKT_C2S_ARENA_ATTACK: 데미지 계산 → S2C AttackResult·HpSync·Die
    void OnHostArenaAttack(SOCKET sock, const Pkt_ArenaAttack& pkt);
    // C2S PKT_C2S_ARENA_ITEM_USE: 포션·버프 처리 → S2C HpSync 등
    void OnHostArenaItemUse(SOCKET sock, const Pkt_ArenaItemUse& pkt);

#pragma endregion

#pragma region Battle Flow

    // ---------- 호스트 세션·상태 전이 ----------

    // S2C PKT_S2C_ARENA_SESSION_APPLY: 전원에게 종료·인벤 동기화 패킷 전송
    void SendArenaSessionApplyToAllPlayers(const Pkt_ArenaRankList& rankPkt);
    // 호스트: 스냅샷 전원 수집 완료 시 S2C 전투 패킷 + CHANGE_STATE ArenaBattle
    void TryStartArenaBattleAfterSnapshots();
    // 호스트: 생존 1명 이하 시 S2C RankList + CHANGE_STATE ArenaResult
    void TryEndArenaBattleIfNeeded();
    // 호스트: 첫 턴 시작(TurnStart + ItemList)
    void StartFirstArenaTurn();
    // 호스트: 공격·아이템 후 다음 생존자 턴으로 진행
    void AdvanceArenaTurnAfterAction();

#pragma endregion

#pragma region Session State

    // ---------- 호스트 권한 검증·턴 (패킷 직접 송수신 아님) ----------

    // 호스트: 스냅샷상 HP > 0 여부(생존 판정)
    bool IsArenaPlayerAlive(const std::string& playerName) const;
    // 호스트: 요청자가 arenaCurrentTurnPlayer와 일치하는지
    bool IsActorsArenaTurn(const std::string& actorName) const;
    // 호스트: 방장 선행 + 접속자 이름 오름차순으로 arenaTurnOrder 구성
    void BuildArenaTurnOrder();
    // 호스트: 전투 시작 시 arenaInitialPlayerSnapshots에 스냅샷 복사(종료 보상·차감용)
    void SaveArenaInitialSnapshots();
    // 호스트: 예정 인원 전원의 C2S 스냅샷 수신 여부
    bool HasAllArenaSnapshots();

    // ---------- 아레나 세션 데이터 ----------

    // C2S PKT_C2S_ARENA_READY: 호스트만 — 도착 신고한 플레이어 이름
    std::set<std::string> arenaLobbyArrived;
    // S2C PKT_S2C_ARENA_LOBBY_STATE: 호스트·게스트 — 마지막 로비 명단 캐시(UI·N/M)
    std::vector<ArenaLobbyPlayerEntry> arenaLobbyDisplay;
    // C2S PKT_C2S_ARENA_PLAYER_SNAPSHOT: 호스트 — 플레이어별 최신 스냅샷 바이트
    std::map<std::string, std::vector<char>> arenaPlayerSnapshots;
    // 호스트: 전투 시작 시점 스냅샷 복본(S2C SESSION_APPLY 인벤 차감 기준)
    std::map<std::string, std::vector<char>> arenaInitialPlayerSnapshots;
    // 호스트: TryStartArenaBattleAfterSnapshots 1회 실행 여부
    bool arenaBattleStarted = false;
    // 호스트: RequestAllArenaPlayerSnapshots 이후 C2S 스냅샷 수집 중
    bool arenaSnapshotCollecting = false;
    // 호스트: 턴 순서(방장 → 접속자 이름 오름차순)
    std::vector<std::string> arenaTurnOrder;
    // 호스트: arenaTurnOrder 내 현재 인덱스
    size_t arenaTurnIndex = 0;
    // 호스트: 지금 행동 가능한 플레이어 이름
    std::string arenaCurrentTurnPlayer;

#pragma endregion
};
