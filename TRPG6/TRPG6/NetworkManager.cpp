#include "NetworkManager.h"
#include "IPCManager.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "DATABASE.h"
#include "ArenaBattleManager.h"
#include "ArenaBattleState.h"
#include <algorithm>

// 패킷 처리 함수=======================================================================================
// 여기서 패킷 종류별로 게임 로직 구현하시면 됩니다.
#pragma region Packet Processing
void NetworkManager::ProcessPacket(SOCKET sock, PacketHeader* header)
{
    switch (header->type)
    {
        case PacketType::PKT_C2S_JOIN: {
            Pkt_Join* pkt = reinterpret_cast<Pkt_Join*>(header);

            // [추가] 방장이 클라이언트의 이름을 맵에 기억해둠
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clientNames[sock] = pkt->name;
            }

            IPCManager::GetInstance().SendPlayerJoin(false, pkt->name);

            Pkt_Join ackPkt(PacketType::PKT_S2C_JOIN_ACK);
            strcpy_s(ackPkt.name, sizeof(ackPkt.name), Client::playerName.c_str());
            send(sock, reinterpret_cast<char*>(&ackPkt), ackPkt.header.size, 0);
            break;
        }

        case PacketType::PKT_S2C_JOIN_ACK: {
            Pkt_Join* pkt = reinterpret_cast<Pkt_Join*>(header);
            IPCManager::GetInstance().SendPlayerJoin(true, pkt->name);
            break;
        }

                                         // [추가] 방장으로부터 누군가 나갔다는 퇴장 패킷을 받았을 때의 처리
        case PacketType::PKT_S2C_LEAVE: {
            Pkt_Join* pkt = reinterpret_cast<Pkt_Join*>(header);
            IPCManager::GetInstance().SendPlayerLeave(pkt->name); // 로컬 창 갱신
            break;
        }

        case PacketType::PKT_CHAT: {
            Pkt_Chat* pkt = reinterpret_cast<Pkt_Chat*>(header);
            IPCManager::GetInstance().SendChat(pkt->sender, pkt->message);

            if (Client::isServer)
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (SOCKET clientSock : connectedClients)
                {
                    if (clientSock != sock)
                    {
                        send(clientSock, reinterpret_cast<char*>(pkt), pkt->header.size, 0);
                    }
                }
            }
            break;
        }

        case PacketType::PKT_S2C_CHANGE_STATE: {
            Pkt_ChangeState* pkt = reinterpret_cast<Pkt_ChangeState*>(header);

            if (!Client::isServer) // 방지책: 방장 본인은 이중 전환 방지
            {
                if (pkt->targetState == EGameState::Start)
                {
                    IPCManager::GetInstance().SendLog("[네트워크] 방장이 게임을 시작했습니다. 인게임으로 진입합니다.");
                    GameManager::GetInstance().SetCurrentState(new GameStartState());
                }
                else if (pkt->targetState == EGameState::ArenaBattle){
                    GameManager::GetInstance().SetCurrentState(new ArenaBattleState());
                }
                    
            }
            break;
        }
        
        case PacketType::PKT_C2S_ARENA_READY:{
            if (!Client::isServer)break; // 호스트만 처리
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                readyCount++;
            }
            IPCManager::GetInstance().SendLog("아레나 준비 완료: " + std::to_string(readyCount)
                + "/" + std::to_string(Server::connectedPlayersCount));

            // 전원 레디 상태면 ArenaBattle로 상태전환
            if (readyCount >= Server::connectedPlayersCount) {
                readyCount = 0;// 초기화
                BroadcastChangeState(EGameState::ArenaBattle);
            }
            break;
        }

        case PacketType::PKT_C2S_ARENA_ITEM_REGISTER: {
            Pkt_ArenaItemRegister* pkt = reinterpret_cast<Pkt_ArenaItemRegister*>(header);
            if (Client::isServer)
            {
                ArenaBattleManager::GetInstance().AddBettedItem(pkt->itemName, pkt->amount);
                IPCManager::GetInstance().SendLog("[아레나] 아이템 등록됨: " + std::string(pkt->itemName) + " (x" + std::to_string(pkt->amount) + ")");
            }
            break;
        }
    }
}
#pragma endregion

// 패킷 전송 함수=======================================================================================
#pragma region Packet Sending Functions
void NetworkManager::SendChatPacket(const std::string& sender, const std::string& message)
{
    Pkt_Chat pkt;
    // 고정 배열 버퍼 오버플로우 방지 복사
    std::string safeSender = sender.substr(0, 31);
    std::string safeMsg = message.substr(0, 127);
    std::strcpy(pkt.sender, safeSender.c_str());
    std::strcpy(pkt.message, safeMsg.c_str());

    // 1. 내 로컬 멀티창에는 즉시 반영
    IPCManager::GetInstance().SendChat(sender, message);

    // 2. 권한별 네트워크 발송
    if (Client::isServer)
    {
        // 내가 호스트라면 접속 중인 모든 클라이언트에게 뿌림
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (SOCKET clientSock : connectedClients)
        {
            send(clientSock, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);
        }
    }
    else
    {
        // 내가 게스트라면 서버에게만 상신
        if (clientSocket != INVALID_SOCKET)
        {
            send(clientSocket, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);
        }
    }
}

void NetworkManager::SendArenaItemRegisterPacket(const std::string& itemName, int count)
{
    Pkt_ArenaItemRegister pkt;
    CopyStringToPacketField(pkt.itemName, sizeof(pkt.itemName), itemName);
    pkt.amount = count;

    if (Client::isServer)
    {
        // 서버(방장)인 경우 바로 ArenaBattleManager에 추가
        ArenaBattleManager::GetInstance().AddBettedItem(itemName, count);
        IPCManager::GetInstance().SendLog("[아레나] 호스트 아이템 등록됨: " + itemName + " (x" + std::to_string(count) + ")");
    }
    else
    {
        // 클라이언트인 경우 서버로 전송
        if (clientSocket != INVALID_SOCKET)
        {
            send(clientSocket, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);
        }
    }
}

void NetworkManager::SendArenaReady() {
    if (Client::isServer) {
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            readyCount++;
        }
        if (readyCount >= Server::connectedPlayersCount) {
            readyCount = 0;
            BroadcastChangeState(EGameState::ArenaBattle);
        }
    }
    else {
        Pkt_ArenaReady pkt;
        if (clientSocket != INVALID_SOCKET) {
            send(clientSocket, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);
        }
    }
    
}
#pragma endregion

// 브로드 캐스팅용 함수=======================================================================================
#pragma region Broadcast Functions
// 플레이어들의 게임 상태 일괄 전환
void NetworkManager::BroadcastChangeState(EGameState stateType)
{
    // 방장이 아니라면 전파 권한 없음
    if (!Client::isServer) return;

    Pkt_ChangeState pkt;
    pkt.targetState = stateType;

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (SOCKET clientSock : connectedClients)
    {
        send(clientSock, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);
    }
}
#pragma endregion

// 기존 구현들=======================================================================================
#pragma region NetworkManager Implementation
void NetworkManager::Init()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        IPCManager::GetInstance().SendLog("[오류] Winsock 초기화 실패");
    }
}

void NetworkManager::Shutdown()
{
    isNetworkRunning = false;
    if (listenSocket != INVALID_SOCKET) closesocket(listenSocket);
    if (clientSocket != INVALID_SOCKET) closesocket(clientSocket);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (SOCKET sock : connectedClients) closesocket(sock);
        connectedClients.clear();
    }

    if (acceptThread.joinable()) acceptThread.join();
    WSACleanup();
}

bool NetworkManager::StartHost(int port)
{
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) return false;

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        return false;
    }

    isNetworkRunning = true;
    acceptThread = std::thread(&NetworkManager::AcceptLoop, this);

    IPCManager::GetInstance().SendLog("[시스템] 서버 호스팅을 시작했습니다. (Port: " + std::to_string(port) + ")");
    return true;
}

bool NetworkManager::ConnectToServer(const std::string& ip, int port)
{
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) return false;

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    IPCManager::GetInstance().SendLog("[네트워크] " + ip + " 서버에 연결 시도 중...");

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    IPCManager::GetInstance().SendLog("[네트워크] 서버 연결 성공!");

    // [추가] 연결 성공 즉시 서버에게 내 이름을 패킷으로 송신
    Pkt_Join pkt(PacketType::PKT_C2S_JOIN);
    strcpy_s(pkt.name, sizeof(pkt.name), Client::playerName.c_str());
    send(clientSocket, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);

    isNetworkRunning = true;
    std::thread(&NetworkManager::ReceiveLoop, this, clientSocket).detach();
    return true;
}

void NetworkManager::AcceptLoop()
{
    while (isNetworkRunning)
    {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET newClient = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);

        if (newClient != INVALID_SOCKET)
        {
            char ipBuf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, INET_ADDRSTRLEN);

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                connectedClients.push_back(newClient);
            }
            IPCManager::GetInstance().SendLog("[네트워크] 클라이언트 접속 성공! IP: " + std::string(ipBuf));

            // [수정] 접속한 클라이언트 전용 수신 백그라운드 스레드 가동
            std::thread(&NetworkManager::ReceiveLoop, this, newClient).detach();
        }
    }
}

void NetworkManager::ReceiveLoop(SOCKET sock)
{
    std::vector<char> recvBuffer;
    char stageBuffer[1024];

    while (isNetworkRunning)
    {
        int bytesRead = recv(sock, stageBuffer, sizeof(stageBuffer), 0);
        if (bytesRead <= 0)
        {
            // [수정] 연결 해제 시의 방어 및 동기화 로직
            if (Client::isServer)
            {
                std::string disconnectedName = "Unknown";

                // 1. 뮤텍스를 걸고 맵에서 나간 사람의 이름을 찾아냄
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    if (clientNames.find(sock) != clientNames.end())
                    {
                        disconnectedName = clientNames[sock];
                        clientNames.erase(sock); // 맵에서 삭제
                    }
                    connectedClients.erase(std::remove(connectedClients.begin(), connectedClients.end(), sock), connectedClients.end());
                }

                // 2. 이름을 성공적으로 찾았다면 퇴장 처리
                if (disconnectedName != "Unknown")
                {
                    IPCManager::GetInstance().SendLog("[네트워크] " + disconnectedName + " 님의 연결이 해제되었습니다.");

                    // 내(방장) 로컬 화면에서 지움
                    IPCManager::GetInstance().SendPlayerLeave(disconnectedName);

                    // 다른 생존한 클라이언트들에게 퇴장 패킷 릴레이
                    Pkt_Join leavePkt(PacketType::PKT_S2C_LEAVE);
                    strcpy_s(leavePkt.name, sizeof(leavePkt.name), disconnectedName.c_str());

                    std::lock_guard<std::mutex> lock(clientsMutex);
                    for (SOCKET cSock : connectedClients)
                    {
                        send(cSock, reinterpret_cast<char*>(&leavePkt), leavePkt.header.size, 0);
                    }
                }
            }
            else
            {
                // 내가 클라이언트인데 recv가 죽었다면 방장이 방을 터트린 것임
                IPCManager::GetInstance().SendLog("[오류] 서버와의 연결이 끊어졌습니다.");
            }

            closesocket(sock);
            break; // 해당 소켓의 수신 스레드 종료
        }

        recvBuffer.insert(recvBuffer.end(), stageBuffer, stageBuffer + bytesRead);

        while (recvBuffer.size() >= sizeof(PacketHeader))
        {
            PacketHeader* header = reinterpret_cast<PacketHeader*>(recvBuffer.data());

            if (recvBuffer.size() >= header->size)
            {
                ProcessPacket(sock, header);
                recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + header->size);
            }
            else
            {
                break;
            }
        }
    }
}
#pragma endregion
