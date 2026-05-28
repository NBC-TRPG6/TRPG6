#include "NetworkManager.h"
#include "ArenaNetworkManager.h"
#include "IPCManager.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "ArenaReadyState.h"
#include "ArenaLobbyState.h"
#include "ArenaBattleState.h"
#include "ArenaWaitState.h"
#include "ArenaResultState.h"
#include "ArenaBattleManager.h"
#include "Player.h"
#include "Item.h"
#include "IGameState.h"
#include "DATABASE.h"
#include "ArenaBattleState.h"
#include "TradeManager.h"
#include <algorithm>
#include <set>
#include "COOPManager.h"
#include "COOPReadyState.h"
#include "COOPSelectJobState.h"
#include "COOPBattleState.h"
#include "COOPRewardState.h"
#include "DieState.h"

#pragma region Arena Helpers

// PKT_S2C_CHANGE_STATE 수신 시 EGameState에 맞는 IGameState 인스턴스 생성
static IGameState* CreateStateFromEGameState(EGameState state)
{
    switch (state)
    {
    case EGameState::Start: return new GameStartState();
    case EGameState::ArenaReady: return new ArenaReadyState();
    case EGameState::ArenaLobby: return new ArenaLobbyState();
    case EGameState::ArenaBattle: return new ArenaBattleState();
    case EGameState::ArenaWait: return new ArenaWaitState();
    case EGameState::ArenaResult: return new ArenaResultState();
    case EGameState::COOPReady: return new COOPReadyState();
    case EGameState::COOPSelectJob: return new COOPSelectJobState();
    case EGameState::COOPBattle: return new COOPBattleState();
    case EGameState::COOPReward: return new COOPRewardState();
    case EGameState::GameOver: return new DieState();

    default: return nullptr;
    }
}

#pragma endregion

#pragma region Packet Processing

void NetworkManager::ProcessPacket(SOCKET sock, PacketHeader* header)
{
    if (ArenaNetworkManager::GetInstance().TryHandlePacket(sock, header))
        return;

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
            IGameState* nextState = CreateStateFromEGameState(pkt->targetState);
            if (nextState == nullptr) break;

                if (pkt->targetState == EGameState::Start)
                {
                    IPCManager::GetInstance().SendLog("[네트워크] 방장이 게임을 시작했습니다. 인게임으로 진입합니다.");
            }
            else if (pkt->targetState == EGameState::ArenaReady)
            {
                IPCManager::GetInstance().SendLog("[아레나] 준비 단계에 진입했습니다.");
            }
            else if (pkt->targetState == EGameState::ArenaLobby)
            {
                IPCManager::GetInstance().SendLog("[아레나] 로비에 입장했습니다.");
            }
            else if (pkt->targetState == EGameState::ArenaBattle)
            {
                IPCManager::GetInstance().SendLog("[아레나] 전투가 시작되었습니다.");
            }
            else if (pkt->targetState == EGameState::ArenaWait)
            {
                IPCManager::GetInstance().SendLog("[아레나] 탈락하여 대기 중입니다.");
            }
            else if (pkt->targetState == EGameState::ArenaResult)
            {
                IPCManager::GetInstance().SendLog("[아레나] 결과 화면으로 이동합니다.");
            }
            else if (pkt->targetState == EGameState::COOPReady)
            {
                IPCManager::GetInstance().SendLog("[COOP] 레이드 대기실에 입장했습니다.");
            }
            else if (pkt->targetState == EGameState::COOPSelectJob)
            {
                IPCManager::GetInstance().SendLog("[COOP] 직업 선택 단계입니다.");
            }
            else if (pkt->targetState == EGameState::COOPBattle)
            {
                IPCManager::GetInstance().SendLog("[COOP] 보스 레이드 전투가 시작되었습니다!");
            }
            else if (pkt->targetState == EGameState::COOPReward)
            {
                IPCManager::GetInstance().SendLog("[COOP] 레이드 성공! 보상을 확인하세요.");
            }
            else if (pkt->targetState == EGameState::GameOver)
            {
                IPCManager::GetInstance().SendLog("[시스템] 모든 플레이어가 사망하여 게임 오버되었습니다.");
            }

            GameManager::GetInstance().SetCurrentState(nextState);
            }
            break;
        }


    case PacketType::PKT_C2S_TRADE_REQUEST:
    {
        Pkt_TradeRequest* pkt = reinterpret_cast<Pkt_TradeRequest*>(header);
        // 방장만 이 패킷을 처리해서 ID를 부여함
        if (Client::isServer)
        {
            TradeManager::GetInstance().Server_HandleRequest(pkt->info);
        }
        break;
    }

    case PacketType::PKT_C2S_TRADE_RESPONSE:
    {
        Pkt_TradeResponse* pkt = reinterpret_cast<Pkt_TradeResponse*>(header);
        // 방장만 이 패킷을 받아서 최종 수락/거절 판정을 내림
        if (Client::isServer)
        {
            TradeManager::GetInstance().Server_HandleResponse(pkt->tradeId, pkt->response);
        }
        break;
    }

    case PacketType::PKT_S2C_TRADE_SYNC:
    {
        Pkt_TradeSync* pkt = reinterpret_cast<Pkt_TradeSync*>(header);
        // 서버가 갱신된 리스트를 보내주면 모두가 내 로컬 리스트를 동기화함
        // (이 과정에서 상태가 1(성공)로 변했다면 아이템 교환 로직도 같이 실행됨)
        TradeManager::GetInstance().SyncTrade(pkt->info);
        break;
    }

    case PacketType::PKT_C2S_COOP_READY: {
        if (Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_C2S_COOP_Ready*>(header);
            COOPManager::GetInstance().SetPlayerReady(GetPlayerNameForSocket(sock), pkt->isReady);
        }
        break;
    }
    case PacketType::PKT_C2S_COOP_UPDATE_STATUS: {
        if (Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_C2S_COOP_Update_Status*>(header);
            COOPManager::GetInstance().UpdatePlayerStatus(pkt->name, pkt->atk, pkt->hp, pkt->job, pkt->isDead);
        }
        break;
    }
    case PacketType::PKT_C2S_COOP_USE_ITEM: {
        if (Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_C2S_COOP_Use_Item*>(header);
            COOPManager::GetInstance().OnPlayerItem(pkt->targetName, pkt->itemName, pkt->amount);
        }
        break;
    }
    case PacketType::PKT_C2S_COOP_USE_ATTACK: {
        if (Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_C2S_COOP_Use_Attack*>(header);
            COOPManager::GetInstance().OnPlayerAttack(pkt->sourceName, pkt->targetName, pkt->amount);
        }
        break;
    }
    case PacketType::PKT_C2S_COOP_USE_BLOCK: {
        if (Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_C2S_COOP_Use_Block*>(header);
            COOPManager::GetInstance().OnPlayerBlock(pkt->sourceName, pkt->targetName);
        }
        break;
    }
    case PacketType::PKT_C2S_COOP_USE_HEAL: {
        if (Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_C2S_COOP_Use_Heal*>(header);
            COOPManager::GetInstance().OnPlayerHeal(pkt->sourceName, pkt->targetName, pkt->amount);
        }
        break;
    }
    case PacketType::PKT_S2C_COOP_UPDATE_STATUS: {
        if (!Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_S2C_COOP_Update_Status*>(header);
            COOPManager::GetInstance().UpdatePlayerStatus(pkt->name, pkt->atk, pkt->hp, pkt->job, pkt->isDead);
        }
        break;
    }
    case PacketType::PKT_S2C_COOP_UPDATE_TURN: {
        if (!Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_S2C_COOP_Update_Turn*>(header);
            COOPManager::GetInstance().UpdateTurn(pkt->targetName, pkt->turn);
        }
        break;
    }
    case PacketType::PKT_S2C_COOP_UPDATE_MONSTER: {
        if (!Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_S2C_COOP_Update_Monster*>(header);
            COOPManager::GetInstance().UpdateMonster(pkt->targetName, pkt->hp);
        }
        break;
    }
    case PacketType::PKT_S2C_COOP_TAKE_ITEM: {
        if (!Client::isServer)
        {
            auto* pkt = reinterpret_cast<Pkt_S2C_COOP_Take_Item*>(header);
            COOPManager::GetInstance().TakeItem(pkt->targetName, pkt->itemName);
        }
        break;
    }

    default:
        break;
    }
}
#pragma endregion


std::string NetworkManager::GetPlayerNameForSocket(SOCKET sock)
{
    if (sock == INVALID_SOCKET) return Client::playerName;

    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = clientNames.find(sock);
    if (it != clientNames.end()) return it->second;
    return Client::playerName;
}

void NetworkManager::SendToServer(const void* data, size_t size)
{
    if (clientSocket != INVALID_SOCKET && data != nullptr && size > 0)
    {
        send(clientSocket, reinterpret_cast<const char*>(data), static_cast<int>(size), 0);
    }
}

void NetworkManager::BroadcastToClients(const void* data, size_t size, SOCKET exceptSock)
{
    if (!Client::isServer || data == nullptr || size == 0) return;

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (SOCKET clientSock : connectedClients)
    {
        if (clientSock != exceptSock)
        {
            send(clientSock, reinterpret_cast<const char*>(data), static_cast<int>(size), 0);
        }
    }
}

void NetworkManager::SendToPlayerByName(const std::string& playerName, const void* data, size_t size)
{
    if (!Client::isServer || data == nullptr || size == 0) return;

    if (playerName == Client::playerName) return;

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& pair : clientNames)
    {
        if (pair.second == playerName)
        {
            send(pair.first, reinterpret_cast<const char*>(data), static_cast<int>(size), 0);
            break;
        }
    }
}

std::set<std::string> NetworkManager::GetConnectedPlayerNames() const
{
    std::set<std::string> names;
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& pair : clientNames)
    {
        names.insert(pair.second);
    }
    return names;
}

void NetworkManager::ApplySyncedStateChange(EGameState stateType)
{
    if (!Client::isServer) return;

    if (stateType == EGameState::ArenaReady)
    {
        ArenaNetworkManager::GetInstance().ResetArenaLobbyArrivalTracking();
    }
    // COOP 레이드 시작 시 데이터 초기화
    if (stateType == EGameState::COOPReady)
    {
        COOPManager::GetInstance().Reset();
    }

    IGameState* nextState = CreateStateFromEGameState(stateType);
    if (nextState != nullptr)
    {
        GameManager::GetInstance().SetCurrentState(nextState);
    }

    BroadcastChangeState(stateType);
}

void NetworkManager::SendStateChangeToPlayer(const std::string& playerName, EGameState stateType)
{
    if (!Client::isServer) return;

    Pkt_ChangeState changePkt;
    changePkt.targetState = stateType;

    if (playerName == Client::playerName)
    {
        IGameState* nextState = CreateStateFromEGameState(stateType);
        if (nextState != nullptr)
        {
            GameManager::GetInstance().SetCurrentState(nextState);
        }
    }
    else
    {
        SendToPlayerByName(playerName, &changePkt, changePkt.header.size);
    }
}

#pragma region Item Implementation
void NetworkManager::SendTradeRequest(const Pkt_TradeRequest& pkt)
{
    if (Client::isServer)
    {
        // 내가 방장인데 내가 신청하는 경우: 스스로에게 바로 처리
        TradeManager::GetInstance().Server_HandleRequest(pkt.info);
    }
    else if (clientSocket != INVALID_SOCKET)
    {
        // 게스트인 경우: 서버로 전송
        send(clientSocket, reinterpret_cast<const char*>(&pkt), pkt.header.size, 0);
    }
}

void NetworkManager::SendTradeResponse(const Pkt_TradeResponse& pkt)
{
    if (Client::isServer)
    {
        TradeManager::GetInstance().Server_HandleResponse(pkt.tradeId, pkt.response);
    }
    else if (clientSocket != INVALID_SOCKET)
    {
        send(clientSocket, reinterpret_cast<const char*>(&pkt), pkt.header.size, 0);
    }
}

void NetworkManager::BroadcastTradeSync(const Pkt_TradeSync& pkt)
{
    if (!Client::isServer) return; // 방장만 브로드캐스트 가능

    // 모든 클라이언트에게 쏘기
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (SOCKET clientSock : connectedClients)
    {
        send(clientSock, reinterpret_cast<const char*>(&pkt), pkt.header.size, 0);
    }

    // 방장 본인의 화면도 즉시 동기화
    TradeManager::GetInstance().SyncTrade(pkt.info);
}
#pragma endregion

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

#pragma endregion


#pragma region COOP
void NetworkManager::SendCOOPReady(bool isReady)
{
    Pkt_C2S_COOP_Ready pkt;
    pkt.isReady = isReady;
    if (Client::isServer)
    {
        COOPManager::GetInstance().SetPlayerReady(Client::playerName, isReady);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::SendCOOPUpdateStatus(const std::string& name, int atk, int hp, int job, bool isDead)
{
    Pkt_C2S_COOP_Update_Status pkt;
    std::strncpy(pkt.name, name.c_str(), sizeof(pkt.name) - 1);
    pkt.atk = atk;
    pkt.hp = hp;
    pkt.job = static_cast<PlayerJob>(job);
    pkt.isDead = isDead;
    if (Client::isServer)
    {
        COOPManager::GetInstance().UpdatePlayerStatus(name, atk, hp, static_cast<PlayerJob>(job), isDead);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::SendCOOPUseItem(const std::string& targetName, const std::string& itemName, int amount)
{
    Pkt_C2S_COOP_Use_Item pkt;
    std::strncpy(pkt.targetName, targetName.c_str(), sizeof(pkt.targetName) - 1);
    std::strncpy(pkt.itemName, itemName.c_str(), sizeof(pkt.itemName) - 1);
    pkt.amount = amount;
    if (Client::isServer)
    {
        COOPManager::GetInstance().OnPlayerItem(targetName, itemName, amount);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::SendCOOPUseAttack(const std::string& sourceName, const std::string& targetName, int amount)
{
    Pkt_C2S_COOP_Use_Attack pkt;
    std::strncpy(pkt.sourceName, sourceName.c_str(), sizeof(pkt.sourceName) - 1);
    std::strncpy(pkt.targetName, targetName.c_str(), sizeof(pkt.targetName) - 1);
    pkt.amount = amount;
    if (Client::isServer)
    {
        COOPManager::GetInstance().OnPlayerAttack(sourceName, targetName, amount);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::SendCOOPUseBlock(const std::string& sourceName, const std::string& targetName)
{
    Pkt_C2S_COOP_Use_Block pkt;
    std::strncpy(pkt.sourceName, sourceName.c_str(), sizeof(pkt.sourceName) - 1);
    std::strncpy(pkt.targetName, targetName.c_str(), sizeof(pkt.targetName) - 1);
    if (Client::isServer)
    {
        COOPManager::GetInstance().OnPlayerBlock(sourceName, targetName);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::SendCOOPUseHeal(const std::string& sourceName, const std::string& targetName, int amount)
{
    Pkt_C2S_COOP_Use_Heal pkt;
    std::strncpy(pkt.sourceName, sourceName.c_str(), sizeof(pkt.sourceName) - 1);
    std::strncpy(pkt.targetName, targetName.c_str(), sizeof(pkt.targetName) - 1);
    pkt.amount = amount;
    if (Client::isServer)
    {
        COOPManager::GetInstance().OnPlayerHeal(sourceName, targetName, amount);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::BroadcastCOOPUpdateStatus(const std::string& name, int atk, int hp, int job, bool isDead)
{
    Pkt_S2C_COOP_Update_Status pkt;
    std::strncpy(pkt.name, name.c_str(), sizeof(pkt.name) - 1);
    pkt.atk = atk; pkt.hp = hp; pkt.job = static_cast<PlayerJob>(job);
    pkt.isDead = isDead;
    BroadcastToClients(&pkt, pkt.header.size);
}

void NetworkManager::BroadcastCOOPUpdateTurn(const std::string& targetName, int turn)
{
    Pkt_S2C_COOP_Update_Turn pkt;
    std::strncpy(pkt.targetName, targetName.c_str(), sizeof(pkt.targetName) - 1);
    pkt.turn = turn;
    BroadcastToClients(&pkt, pkt.header.size);
}

void NetworkManager::BroadcastCOOPUpdateMonster(const std::string& targetName, int hp)
{
    Pkt_S2C_COOP_Update_Monster pkt;
    std::strncpy(pkt.targetName, targetName.c_str(), sizeof(pkt.targetName) - 1);
    pkt.hp = hp;
    BroadcastToClients(&pkt, pkt.header.size);
}

void NetworkManager::BroadcastCOOPTakeItem(const std::string& targetName, const std::string& itemName)
{
    Pkt_S2C_COOP_Take_Item pkt;
    std::strncpy(pkt.targetName, targetName.c_str(), sizeof(pkt.targetName) - 1);
    std::strncpy(pkt.itemName, itemName.c_str(), sizeof(pkt.itemName) - 1);
    BroadcastToClients(&pkt, pkt.header.size);
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
        clientNames.clear();
    }

    ArenaBattleManager::GetInstance().ResetSession();
    ArenaNetworkManager::GetInstance().ClearSessionData();

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

                    ArenaNetworkManager::GetInstance().OnPlayerDisconnected();
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
