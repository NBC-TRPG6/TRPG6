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

// Winsock2 라이브러리 링크
#pragma comment(lib, "ws2_32.lib")

class NetworkManager {
public:
    static NetworkManager& GetInstance()
    {
        static NetworkManager instance;
        return instance;
    }

    void Init(); // 프로그램 시작 시 WSAStartup 호출
    void Shutdown(); // WSACleanup 및 소켓 정리

    // 방장(서버) 기능
    bool StartHost(int port);

    // 참가자(클라이언트) 기능
    bool ConnectToServer(const std::string& ip, int port);

    void SendChatPacket(const std::string& sender, const std::string& message);

    // 방장이 모든 클라이언트의 화면 상태를 강제로 전환시키는 함수
    void BroadcastChangeState(EGameState stateType);

private:
    NetworkManager() = default;
    ~NetworkManager() { Shutdown(); }

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET; // 내가 클라이언트일 때 사용하는 소켓
    std::vector<SOCKET> connectedClients; // 내가 서버일 때 접속한 클라이언트들
    std::mutex clientsMutex;

    std::thread acceptThread;
    std::atomic<bool> isNetworkRunning{ false };

    // 소켓 ID로 플레이어 이름을 찾기 위한 컨테이너
    std::map<SOCKET, std::string> clientNames;

    // 방장 전용: 백그라운드에서 클라이언트들의 접속을 무한 대기하는 함수
    void AcceptLoop();

    // 백그라운드 패킷 수신 루프 (소켓마다 독립 실행)
    void ReceiveLoop(SOCKET sock);

    // 조립 완료된 완성본 패킷 처리기
    void ProcessPacket(SOCKET sock, PacketHeader* header);
};
