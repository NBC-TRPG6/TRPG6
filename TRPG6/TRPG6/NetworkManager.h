// NetworkManager.h
#pragma once

// ★ 무조건 winsock2.h 및 windows.h 관련 인클루드보다 위에 선언되어야 합니다!
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include "Packet.h"
#include "DATABASE.h"

class Player;

// Winsock2 라이브러리 링크
#pragma comment(lib, "ws2_32.lib")

/*
* 멀티 플레이 구현 시 체크리스트
* 1. 패킷 구조체 정의 (Packet.h)
* 2. 패킷 처리 함수 구현 (NetworkManager::ProcessPacket) -> 권한별로 서버/클라이언트에서 각각 구현
* 3. 패킷 전송 함수 구현 (NetworkManager::SendXXXPacket) -> 권한별로 서버/클라이언트에서 각각 구현
* 4. (선택) 브로드캐스팅용 함수 구현 (NetworkManager::BroadcastXXX)
*/

class NetworkManager {
// 패킷 처리 함수=======================================================================================
public:
    void ProcessPacket(SOCKET sock, PacketHeader* header);

// 패킷 전송 함수=======================================================================================
public:
    void SendChatPacket(const std::string& sender, const std::string& message);

#pragma region Arena
    // C2S: 보상(베팅) 아이템 등록 — ArenaReady / ArenaBetting 단계
    void SendArenaItemRegisterPacket(const std::string& itemName, int count);
    // C2S: 준비 완료 → 호스트가 해당 플레이어만 ArenaLobby로 전환
    void SendArenaReadyPacket();
    // C2S: 전투 입장 스냅샷(스탯+인벤 최대 14슬롯, 가변 길이) 전송
    void SendArenaPlayerSnapshotPacket(Player* player);
    // C2S: 공격 요청(대상 이름만, 공격자는 소켓/호스트 이름으로 판별)
    void SendArenaAttackPacket(const std::string& targetName);
    // C2S: 아이템 사용(targetName 비우면 자기 자신)
    void SendArenaItemUsePacket(const std::string& itemName, const std::string& targetName);

    // 호스트 전용: 로컬 상태 전환 + 접속 클라이언트 전원에게 PKT_S2C_CHANGE_STATE
    void ApplySyncedStateChange(EGameState stateType);
    // 호스트 전용: 지정 플레이어 1명만 EGameState 전환(탈락 ArenaWait 등)
    void SendStateChangeToPlayer(const std::string& playerName, EGameState stateType);

    // S2C: 전원 요약 스탯 UI용 플레이어 목록
    void BroadcastArenaPlayerList(const Pkt_ArenaPlayerList& pkt);
    // S2C: 현재 턴 플레이어 이름
    void BroadcastArenaTurnStart(const std::string& turnPlayerName);
    // S2C: 서버 계산 공격 결과(데미지 포함)
    void BroadcastArenaAttackResult(const Pkt_ArenaAttackResult& pkt);
    // S2C: HP 절대값 동기화(전투 중 재스냅샷 없음, 방법 A)
    void BroadcastArenaHpSync(const Pkt_ArenaHpSync& pkt);
    // S2C: 턴 보유자 보유 아이템 목록(header.size는 slotCount에 맞게 설정)
    void BroadcastArenaItemList(const Pkt_ArenaItemList& pkt);
    // S2C: 사망 통지
    void BroadcastArenaDie(const std::string& playerName);
    // S2C: 아레나 종료 순위
    void BroadcastArenaRankList(const Pkt_ArenaRankList& pkt);

#pragma endregion

// 브로드 캐스팅 함수=======================================================================================
public:
    void BroadcastChangeState(EGameState stateType);

// 네트워크 매니저의 데이터=======================================================================================
private:
    // 소켓 ID로 플레이어 이름을 찾기 위한 컨테이너
    std::map<SOCKET, std::string> clientNames;
    int readyCount = 0;

    // 호스트: 플레이어별 C2S 스냅샷 원본(가변 패킷 바이트)
    std::map<std::string, std::vector<char>> arenaPlayerSnapshots;
    // 호스트: 전원 스냅샷 수집 후 전투 시작 1회만 수행
    bool arenaBattleStarted = false;

    // 소켓 → 플레이어 이름(INVALID_SOCKET이면 호스트 본인)
    std::string GetPlayerNameForSocket(SOCKET sock);
    // 게스트: 서버(방장)로 패킷 1건 전송
    void SendToServer(const void* data, size_t size);
    // 호스트: 접속 클라이언트 전원에 전송(exceptSock 제외 가능)
    void BroadcastToClients(const void* data, size_t size, SOCKET exceptSock = INVALID_SOCKET);
    // 호스트: 이름으로 소켓 찾아 1명에게만 전송
    void SendToPlayerByName(const std::string& playerName, const void* data, size_t size);

    // 호스트: PKT_C2S_ARENA_ITEM_REGISTER 처리 → ArenaBattleManager 베팅 등록
    void OnHostArenaItemRegister(SOCKET sock, const Pkt_ArenaItemRegister& pkt);
    // 호스트: PKT_C2S_ARENA_READY 처리 → SendStateChangeToPlayer(ArenaLobby)
    void OnHostArenaReady(SOCKET sock);
    // 호스트: PKT_C2S_ARENA_PLAYER_SNAPSHOT 저장 후 전원 수집 시 전투 시작
    void OnHostArenaPlayerSnapshot(SOCKET sock, const char* packetData, size_t packetSize);
    // 호스트: PKT_C2S_ARENA_ATTACK 처리 → 데미지·AttackResult·HpSync·Die
    void OnHostArenaAttack(SOCKET sock, const Pkt_ArenaAttack& pkt);
    // 호스트: PKT_C2S_ARENA_ITEM_USE 처리 → 회복 등 후 HpSync
    void OnHostArenaItemUse(SOCKET sock, const Pkt_ArenaItemUse& pkt);

    // 호스트: 스냅샷 전원 도착 시 PlayerList·TurnStart·ItemList 후 ArenaBattle
    void TryStartArenaBattleAfterSnapshots();
    // 호스트: 아레나 세션 맵/플래그 초기화(Shutdown 등)
    void ClearArenaSessionData();
    // 호스트: 호스트+접속자 전원 스냅샷 수신 여부
    bool HasAllArenaSnapshots();


// 네트워크 매니저의 핵심 기능=======================================================================================
#pragma region Core
public:
    static NetworkManager& GetInstance()
    {
        static NetworkManager instance;
        return instance;
    }

    void Init(); // 프로그램 시작 시 WSAStartup 호출
    void Shutdown(); // WSACleanup 및 소켓 정리

    bool StartHost(int port); // 방장(서버) 기능
    bool ConnectToServer(const std::string& ip, int port); // 참가자(클라이언트) 기능

private:
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET; // 내가 클라이언트일 때 사용하는 소켓
    std::vector<SOCKET> connectedClients; // 내가 서버일 때 접속한 클라이언트들
    std::mutex clientsMutex;
    std::thread acceptThread;
    std::atomic<bool> isNetworkRunning{ false };

private:
    NetworkManager() = default;
    ~NetworkManager() { Shutdown(); }

    // 방장 전용: 백그라운드에서 클라이언트들의 접속을 무한 대기하는 함수
    void AcceptLoop();

    // 백그라운드 패킷 수신 루프 (소켓마다 독립 실행)
    void ReceiveLoop(SOCKET sock);
};
#pragma endregion
