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
#include <set>
#include <atomic>
#include <mutex>
#include "Packet.h"
#include "DATABASE.h"
#include "Item.h"
#include "ArenaNetworkManager.h"

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

    // S2C PKT_S2C_CHANGE_STATE: 호스트 로컬 전환 + 접속자 전원 동일 EGameState 동기화
    void ApplySyncedStateChange(EGameState stateType);
    // S2C PKT_S2C_CHANGE_STATE: 지정 1명만 상태 전환(예: 탈락 → ArenaWait)
    void SendStateChangeToPlayer(const std::string& playerName, EGameState stateType);

    // C2S/S2C: 소켓 → 플레이어 이름(INVALID_SOCKET이면 호스트 본인)
    std::string GetPlayerNameForSocket(SOCKET sock);
    // C2S: 게스트 → 호스트 1건 전송
    void SendToServer(const void* data, size_t size);
    // S2C: 호스트 → 접속 클라이언트 전원(exceptSock 제외 가능)
    void BroadcastToClients(const void* data, size_t size, SOCKET exceptSock = INVALID_SOCKET);
    // S2C: 호스트 → 이름 일치 1명만 전송
    void SendToPlayerByName(const std::string& playerName, const void* data, size_t size);
    // 호스트: 접속 중인 게스트 플레이어 이름 집합
    std::set<std::string> GetConnectedPlayerNames() const;

    // COOP 대기실 UI: 아레나와 동일한 참가 예정 인원 수
    int GetExpectedArenaPlayerCount() const
    {
        return ArenaNetworkManager::GetInstance().GetExpectedArenaPlayerCount();
    }

#pragma region Item
    void SendTradeRequest(const Pkt_TradeRequest& pkt);
    void SendTradeResponse(const Pkt_TradeResponse& pkt);
    void BroadcastTradeSync(const Pkt_TradeSync& pkt);
#pragma endregion

#pragma region Gold
    void HandleGoldTradeRequest(SOCKET sock, struct Pkt_GoldTradeRequest* pkt);
    void HandleGoldTradeAck(struct Pkt_GoldTradeAck* pkt);
    void SendGoldTradeRequest(const std::string& receiverName, int32_t amount);
    SOCKET GetClientSocket() const { return clientSocket; }
#pragma endregion

#pragma region COOP
    // COOP Send Functions
    void SendCOOPReady(bool isReady);
    void SendCOOPUpdateStatus(const std::string& name, int atk, int hp, int maxhp, int job, bool isDead);
    void SendCOOPUseItem(const std::string& targetName, const std::string& itemName, int amount);
    void SendCOOPUseAttack(const std::string& sourceName, const std::string& targetName, int amount);
    void SendCOOPUseBlock(const std::string& sourceName, const std::string& targetName);
    void SendCOOPUseHeal(const std::string& sourceName, const std::string& targetName, int amount);
    
    // COOP Broadcast Functions
    void BroadcastCOOPUpdateStatus(const std::string& name, int atk, int hp, int maxhp, int job, bool isDead);
    void BroadcastCOOPUpdateTurn(const std::string& targetName, int turn);
    void BroadcastCOOPUpdateMonster(const std::string& targetName, int hp);
    void BroadcastCOOPTakeItem(const std::string& targetName, const std::string& itemName);
#pragma endregion

// 브로드 캐스팅 함수=======================================================================================
public:
    void BroadcastChangeState(EGameState stateType);

// 네트워크 매니저의 데이터=======================================================================================
private:
    // 소켓 ID로 플레이어 이름을 찾기 위한 컨테이너
    std::map<SOCKET, std::string> clientNames;
    
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

    int GetConnectedClientCount() const {
        std::lock_guard<std::mutex> lock(clientsMutex);
        return (int)connectedClients.size();
    }

private:
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET; // 내가 클라이언트일 때 사용하는 소켓
    std::vector<SOCKET> connectedClients; // 내가 서버일 때 접속한 클라이언트들
    mutable std::mutex clientsMutex;
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
