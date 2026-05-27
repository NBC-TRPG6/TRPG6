#include "NetworkManager.h"
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

static ArenaBattleState* GetArenaBattleState()
{
    return dynamic_cast<ArenaBattleState*>(GameManager::GetInstance().GetCurrentState());
}

static ArenaLobbyState* GetArenaLobbyState()
{
    return dynamic_cast<ArenaLobbyState*>(GameManager::GetInstance().GetCurrentState());
}

static ArenaWaitState* GetArenaWaitState()
{
    return dynamic_cast<ArenaWaitState*>(GameManager::GetInstance().GetCurrentState());
}

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

    case PacketType::PKT_C2S_ARENA_ITEM_REGISTER: {
        if (!Client::isServer) break;
        OnHostArenaItemRegister(sock, *reinterpret_cast<Pkt_ArenaItemRegister*>(header));
        break;
    }

    case PacketType::PKT_C2S_ARENA_READY: {
        if (!Client::isServer) break;
        OnHostArenaLobbyArrived(sock);
        break;
    }

    case PacketType::PKT_C2S_ARENA_PLAYER_SNAPSHOT: {
        if (!Client::isServer) break;
        OnHostArenaPlayerSnapshot(sock, reinterpret_cast<const char*>(header), header->size);
        break;
    }

    case PacketType::PKT_C2S_ARENA_ATTACK: {
        if (!Client::isServer) break;
        OnHostArenaAttack(sock, *reinterpret_cast<Pkt_ArenaAttack*>(header));
        break;
    }

    case PacketType::PKT_C2S_ARENA_ITEM_USE: {
        if (!Client::isServer) break;
        OnHostArenaItemUse(sock, *reinterpret_cast<Pkt_ArenaItemUse*>(header));
        break;
    }

                                           // ---------- 아레나 S2C (게스트: UI + 관전 캐시) ----------
    case PacketType::PKT_S2C_ARENA_PLAYER_LIST: {
        if (Client::isServer) break;
        auto* pkt = reinterpret_cast<Pkt_ArenaPlayerList*>(header);
        ArenaBattleManager::GetInstance().OnSpectatorPlayerList(*pkt);
        NotifyArenaBattlePlayerList(*pkt);
        IPCManager::GetInstance().SendLog(
            "[아레나] 플레이어 목록 수신 (" + std::to_string(pkt->playerCount) + "명)");
        break;
    }

    case PacketType::PKT_S2C_ARENA_TURN_START: {
        if (Client::isServer) break;
        auto* pkt = reinterpret_cast<Pkt_ArenaTurnStart*>(header);
        ArenaBattleManager::GetInstance().OnSpectatorTurnStart(pkt->turnPlayerName);
        NotifyArenaBattleTurnStart(pkt->turnPlayerName);
        IPCManager::GetInstance().SendLog(
            "[아레나] 턴 시작: " + std::string(pkt->turnPlayerName));
        break;
    }

    case PacketType::PKT_S2C_ARENA_ATTACK_RESULT: {
        if (Client::isServer) break;
        auto* pkt = reinterpret_cast<Pkt_ArenaAttackResult*>(header);
        ArenaBattleManager::GetInstance().OnSpectatorAttackResult(*pkt);
        NotifyArenaBattleAttackResult(*pkt);
        IPCManager::GetInstance().SendLog(
            "[아레나] " + std::string(pkt->attackerName) + " -> " +
            std::string(pkt->targetName) + " 데미지 " + std::to_string(pkt->damage));
        break;
    }

        case PacketType::PKT_S2C_ARENA_ITEM_RESULT:
        {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaItemResult*>(header);

            NotifyArenaItemResult(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] " + std::string(pkt->userName) + " 사용 " +
                std::string(pkt->itemName) + " -> 효과: " + std::to_string(pkt->value));
            break;
        }

        case PacketType::PKT_S2C_ARENA_HP_SYNC: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaHpSync*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorHpSync(*pkt);
            NotifyArenaBattleHpSync(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] HP: " + std::string(pkt->playerName) +
                " " + std::to_string(pkt->currentHp) + "/" + std::to_string(pkt->maxHp));
            break;
        }

    case PacketType::PKT_S2C_ARENA_ITEM_LIST: {
        if (Client::isServer) break;
        auto* pkt = reinterpret_cast<Pkt_ArenaItemList*>(header);
        NotifyArenaBattleItemList(*pkt);
        IPCManager::GetInstance().SendLog(
            "[아레나] 아이템 목록: " + std::string(pkt->ownerName) +
            " (" + std::to_string(pkt->slotCount) + "종)");
        break;
    }

    case PacketType::PKT_S2C_ARENA_DIE: {
        if (Client::isServer) break;
        auto* pkt = reinterpret_cast<Pkt_ArenaDie*>(header);
        ArenaBattleManager::GetInstance().OnSpectatorDie(pkt->playerName);
        NotifyArenaBattleDie(pkt->playerName);
        IPCManager::GetInstance().SendLog("[아레나] 사망: " + std::string(pkt->playerName));
        break;
    }

    case PacketType::PKT_S2C_ARENA_RANK_LIST: {
        if (Client::isServer) break;
        auto* pkt = reinterpret_cast<Pkt_ArenaRankList*>(header);
        ArenaBattleManager::GetInstance().OnSpectatorRankList(*pkt);
        IPCManager::GetInstance().SendLog(
            "[아레나] 순위 수신 (" + std::to_string(pkt->entryCount) + "명)");
        break;
    }

        case PacketType::PKT_S2C_ARENA_REWARD_POOL: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaRewardPool*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorRewardPool(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] 보상 풀 수신 (" + std::to_string(pkt->slotCount) + "종)");
            break;
        }

        case PacketType::PKT_S2C_ARENA_SESSION_APPLY: {
            if (Client::isServer) break;

        Player* player = GameManager::GetInstance().GetPlayer();
        if (player != nullptr)
        {
            ApplyArenaSessionToLocalPlayer(
                player,
                reinterpret_cast<const char*>(header),
                header->size);
        }
        break;
    }

    case PacketType::PKT_S2C_ARENA_LOBBY_STATE: {
        if (Client::isServer) break;

        auto* pkt = reinterpret_cast<Pkt_ArenaLobbyState*>(header);
        ApplyArenaLobbyStateCache(*pkt);
        IPCManager::GetInstance().SendLog(
            "[아레나] 로비 상태 수신 (" + std::to_string(pkt->playerCount) + "명)");
        break;
    }

        case PacketType::PKT_S2C_ARENA_BET_REFUND: {
            if (Client::isServer) break;

            auto* pkt = reinterpret_cast<Pkt_ArenaBetRefund*>(header);
            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                ApplyArenaBetRefundToLocalPlayer(player, *pkt);
            }
            break;
        }

        case PacketType::PKT_S2C_ARENA_SNAPSHOT_REQUEST: {
            if (Client::isServer) break;

        if (GetArenaLobbyState() == nullptr)
        {
            IPCManager::GetInstance().SendLog(
                "[아레나] 로비가 아닌 상태에서 스냅샷 요청 무시");
            break;
        }

        Player* player = GameManager::GetInstance().GetPlayer();
        if (player != nullptr)
        {
            SendArenaPlayerSnapshotPacket(player);
            IPCManager::GetInstance().SendLog("[아레나] 방장 요청으로 스냅샷 전송");
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

#pragma region Arena Implementation

// ---------------------------------------------------------------------------
// 전송·소켓 헬퍼
// ---------------------------------------------------------------------------

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

void NetworkManager::ClearArenaSessionData()
{
    arenaPlayerSnapshots.clear();
    arenaInitialPlayerSnapshots.clear();
    arenaBattleStarted = false;
    arenaSnapshotCollecting = false;
    arenaTurnOrder.clear();
    arenaTurnIndex = 0;
    arenaCurrentTurnPlayer.clear();
    ResetArenaLobbyArrivalTracking();
}

bool NetworkManager::IsArenaPlayerAlive(const std::string& playerName) const
{
    auto it = arenaPlayerSnapshots.find(playerName);
    if (it == arenaPlayerSnapshots.end()) return false;

    const auto* hdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(it->second.data());
    return hdr->hp > 0;
}

bool NetworkManager::IsActorsArenaTurn(const std::string& actorName) const
{
    return arenaBattleStarted
        && !arenaCurrentTurnPlayer.empty()
        && actorName == arenaCurrentTurnPlayer
        && IsArenaPlayerAlive(actorName);
}

void NetworkManager::BuildArenaTurnOrder()
{
    arenaTurnOrder.clear();
    arenaTurnOrder.push_back(Client::playerName);

    std::vector<std::string> others;
    for (const auto& pair : arenaPlayerSnapshots)
    {
        if (pair.first != Client::playerName)
        {
            others.push_back(pair.first);
        }
    }
    std::sort(others.begin(), others.end());
    arenaTurnOrder.insert(arenaTurnOrder.end(), others.begin(), others.end());
}

void NetworkManager::BuildArenaPlayerListPacket(Pkt_ArenaPlayerList& out) const
{
    out = Pkt_ArenaPlayerList();
    uint8_t idx = 0;

    for (const auto& pair : arenaPlayerSnapshots)
    {
        if (idx >= MAX_ARENA_PLAYERS) break;

        const auto* hdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(pair.second.data());
        CopyStringToPacketField(out.players[idx].playerName, sizeof(out.players[idx].playerName), pair.first);
        out.players[idx].hp = hdr->hp;
        out.players[idx].maxHp = hdr->maxHp;
        out.players[idx].attack = hdr->attack;
        out.players[idx].level = hdr->level;
        out.players[idx].isAlive = hdr->hp > 0 ? 1 : 0;
        ++idx;
    }
    out.playerCount = idx;
}

void NetworkManager::BroadcastArenaPlayerListFromSnapshots()
{
    if (!Client::isServer) return;

    Pkt_ArenaPlayerList listPkt;
    BuildArenaPlayerListPacket(listPkt);
    BroadcastArenaPlayerList(listPkt);
}

void NetworkManager::SaveArenaInitialSnapshots()
{
    arenaInitialPlayerSnapshots.clear();
    for (const auto& pair : arenaPlayerSnapshots)
    {
        arenaInitialPlayerSnapshots[pair.first] = pair.second;
    }
}

int NetworkManager::GetArenaRankForPlayer(const Pkt_ArenaRankList& rankPkt, const std::string& playerName)
{
    for (uint8_t i = 0; i < rankPkt.entryCount && i < MAX_ARENA_PLAYERS; ++i)
    {
        if (std::string(rankPkt.entries[i].playerName) == playerName)
        {
            return rankPkt.entries[i].rank;
        }
    }
    return 0;
}

bool NetworkManager::BuildArenaSessionApplyPacket(const std::string& playerName,
    const Pkt_ArenaRankList& rankPkt, std::vector<char>& outBuffer) const
{
    outBuffer.clear();

    auto finalIt = arenaPlayerSnapshots.find(playerName);
    auto initialIt = arenaInitialPlayerSnapshots.find(playerName);
    if (finalIt == arenaPlayerSnapshots.end() || initialIt == arenaInitialPlayerSnapshots.end())
    {
        return false;
    }

    const auto* finalHdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(finalIt->second.data());
    const auto* initialHdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(initialIt->second.data());
    const ArenaItemSlot* finalSlots = GetArenaSnapshotItems(finalHdr);
    const ArenaItemSlot* initialSlots = GetArenaSnapshotItems(initialHdr);

    std::vector<ArenaItemSlot> battleSlots;
    battleSlots.reserve(initialHdr->itemSlotCount);

    for (uint8_t i = 0; i < initialHdr->itemSlotCount && i < MAX_ARENA_ITEM_SLOTS; ++i)
    {
        ArenaItemSlot slot = initialSlots[i];
        slot.count = 0;

        for (uint8_t j = 0; j < finalHdr->itemSlotCount && j < MAX_ARENA_ITEM_SLOTS; ++j)
        {
            if (std::string(finalSlots[j].itemName) == std::string(initialSlots[i].itemName))
            {
                slot.count = finalSlots[j].count;
                break;
            }
        }
        battleSlots.push_back(slot);
    }

    std::vector<ArenaItemSlot> rewardSlots;
    if (GetArenaRankForPlayer(rankPkt, playerName) == 1)
    {
        for (const ArenaItemSlot& bet : ArenaBattleManager::GetInstance().GetBettedItems())
        {
            if (rewardSlots.size() >= MAX_ARENA_ITEM_SLOTS) break;
            if (bet.count <= 0) continue;
            rewardSlots.push_back(bet);
        }
    }

    const uint8_t battleCount = static_cast<uint8_t>(battleSlots.size());
    const uint8_t rewardCount = static_cast<uint8_t>(rewardSlots.size());
    const size_t totalSize = ArenaSessionApplyPacketSize(battleCount, rewardCount);
    outBuffer.resize(totalSize);

    auto* hdr = reinterpret_cast<Pkt_ArenaSessionApplyHeader*>(outBuffer.data());
    hdr->header.size = static_cast<uint16_t>(totalSize);
    hdr->header.type = PacketType::PKT_S2C_ARENA_SESSION_APPLY;
    hdr->hp = finalHdr->hp;
    hdr->maxHp = finalHdr->maxHp;
    hdr->attack = finalHdr->attack;
    hdr->battleSlotCount = battleCount;
    hdr->rewardSlotCount = rewardCount;

    if (battleCount > 0)
    {
        std::memcpy(outBuffer.data() + ArenaSessionApplyHeaderSize(),
            battleSlots.data(),
            battleCount * sizeof(ArenaItemSlot));
    }

    if (rewardCount > 0)
    {
        std::memcpy(outBuffer.data() + ArenaSessionApplyHeaderSize() + battleCount * sizeof(ArenaItemSlot),
            rewardSlots.data(),
            rewardCount * sizeof(ArenaItemSlot));
    }

    return true;
}

void NetworkManager::SendArenaSessionApplyToAllPlayers(const Pkt_ArenaRankList& rankPkt)
{
    if (!Client::isServer) return;

    std::set<std::string> playerNames;
    for (const auto& pair : arenaPlayerSnapshots)
    {
        playerNames.insert(pair.first);
    }

    for (const std::string& name : playerNames)
    {
        std::vector<char> buffer;
        if (!BuildArenaSessionApplyPacket(name, rankPkt, buffer)) continue;

        if (name == Client::playerName)
        {
            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                ApplyArenaSessionToLocalPlayer(player, buffer.data(), buffer.size());
            }
        }
        else
        {
            SendToPlayerByName(name, buffer.data(), buffer.size());
        }
    }

    ArenaBattleManager::GetInstance().ClearBettedItems();
}

void NetworkManager::BuildArenaRewardPoolPacket(Pkt_ArenaRewardPool& out) const
{
    out = Pkt_ArenaRewardPool();

    const std::vector<ArenaItemSlot>& bettedItems =
        ArenaBattleManager::GetInstance().GetBettedItems();

    uint8_t idx = 0;
    for(const ArenaItemSlot& bet : bettedItems)
    {
        if (idx >= MAX_ARENA_ITEM_SLOTS) break;
        if (bet.count <= 0) continue;
        out.slots[idx] = bet;
        ++idx;
    }

    out.slotCount = idx;
    out.header.size = static_cast<uint16_t>(
        offsetof(Pkt_ArenaRewardPool, slots) + idx * sizeof(ArenaItemSlot));
}

bool NetworkManager::BuildArenaItemListPacket(const std::string& ownerName, Pkt_ArenaItemList& out) const
{
    auto it = arenaPlayerSnapshots.find(ownerName);
    if (it == arenaPlayerSnapshots.end()) return false;

    const auto* hdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(it->second.data());
    const ArenaItemSlot* slots = GetArenaSnapshotItems(hdr);

    out = Pkt_ArenaItemList();
    CopyStringToPacketField(out.ownerName, sizeof(out.ownerName), ownerName);
    out.slotCount = hdr->itemSlotCount;
    if (out.slotCount > MAX_ARENA_ITEM_SLOTS) out.slotCount = MAX_ARENA_ITEM_SLOTS;

    for (uint8_t i = 0; i < out.slotCount; ++i)
    {
        out.slots[i] = slots[i];
    }
    out.header.size = static_cast<uint16_t>(
        offsetof(Pkt_ArenaItemList, slots) + out.slotCount * sizeof(ArenaItemSlot));
    return true;
}

void NetworkManager::BroadcastArenaTurnAndItems(const std::string& playerName)
{
    if (!Client::isServer) return;

    arenaCurrentTurnPlayer = playerName;
    BroadcastArenaTurnStart(playerName);

    Pkt_ArenaItemList itemPkt;
    if (BuildArenaItemListPacket(playerName, itemPkt))
    {
        BroadcastArenaItemList(itemPkt);
    }
}

void NetworkManager::BroadcastArenaRewardPool()
{
    if (!Client::isServer) return;

    Pkt_ArenaRewardPool pkt;
    BuildArenaRewardPoolPacket(pkt);

    ArenaBattleManager::GetInstance().OnSpectatorRewardPool(pkt);
    BroadcastToClients(&pkt,pkt.header.size);
}

void NetworkManager::StartFirstArenaTurn()
{
    if (!Client::isServer || arenaTurnOrder.empty()) return;

    for (size_t i = 0; i < arenaTurnOrder.size(); ++i)
    {
        if (IsArenaPlayerAlive(arenaTurnOrder[i]))
        {
            arenaTurnIndex = i;
            BroadcastArenaTurnAndItems(arenaTurnOrder[i]);
            return;
        }
    }
}

void NetworkManager::AdvanceArenaTurnAfterAction()
{
    if (!Client::isServer || arenaTurnOrder.empty()) return;

    const size_t playerCount = arenaTurnOrder.size();
    for (size_t step = 1; step <= playerCount; ++step)
    {
        const size_t nextIdx = (arenaTurnIndex + step) % playerCount;
        const std::string& candidate = arenaTurnOrder[nextIdx];
        if (IsArenaPlayerAlive(candidate))
        {
            arenaTurnIndex = nextIdx;
            BroadcastArenaTurnAndItems(candidate);
            return;
        }
    }
}

void NetworkManager::ResetArenaLobbyArrivalTracking()
{
    arenaLobbyArrived.clear();
    arenaLobbyDisplay.clear();
}

std::set<std::string> NetworkManager::CollectExpectedArenaPlayerNames() const
{
    std::set<std::string> expected;
    expected.insert(Client::playerName);

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& pair : clientNames)
    {
        expected.insert(pair.second);
    }
    return expected;
}

int NetworkManager::GetExpectedArenaPlayerCount() const
{
    if (!arenaLobbyDisplay.empty())
    {
        return static_cast<int>(arenaLobbyDisplay.size());
    }
    return static_cast<int>(CollectExpectedArenaPlayerNames().size());
}

int NetworkManager::GetArenaLobbyArrivedCount() const
{
    if (!arenaLobbyDisplay.empty())
    {
        int arrived = 0;
        for (const ArenaLobbyPlayerEntry& entry : arenaLobbyDisplay)
        {
            if (entry.hasArrived != 0) ++arrived;
        }
        return arrived;
    }

    if (Client::isServer)
    {
        return static_cast<int>(arenaLobbyArrived.size());
    }
    return 0;
}

bool NetworkManager::HasAllPlayersInArenaLobby() const
{
    if (!arenaLobbyDisplay.empty())
    {
        for (const ArenaLobbyPlayerEntry& entry : arenaLobbyDisplay)
        {
            if (entry.hasArrived == 0) return false;
        }
        return true;
    }

    const std::set<std::string> expected = CollectExpectedArenaPlayerNames();
    if (expected.empty()) return false;

    for (const std::string& name : expected)
    {
        if (arenaLobbyArrived.find(name) == arenaLobbyArrived.end())
        {
            return false;
        }
    }
    return true;
}

const std::vector<ArenaLobbyPlayerEntry>& NetworkManager::GetArenaLobbyPlayers() const
{
    return arenaLobbyDisplay;
}

// 호스트 이름 + clientNames 전원이 arenaPlayerSnapshots에 있는지 확인
bool NetworkManager::HasAllArenaSnapshots()
{
    std::set<std::string> expected;
    expected.insert(Client::playerName);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto& pair : clientNames)
        {
            expected.insert(pair.second);
        }
    }

    if (arenaPlayerSnapshots.size() < expected.size()) return false;

    for (const std::string& name : expected)
    {
        if (arenaPlayerSnapshots.find(name) == arenaPlayerSnapshots.end()) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 호스트 C2S 처리
// ---------------------------------------------------------------------------

void NetworkManager::OnHostArenaItemRegister(SOCKET sock, const Pkt_ArenaItemRegister& pkt)
{
    std::string playerName = GetPlayerNameForSocket(sock);

    ArenaItemSlot slot{};
    CopyStringToPacketField(slot.itemName, sizeof(slot.itemName), pkt.itemName);
    slot.count = pkt.amount;
    slot.itemType = pkt.itemType;
    slot.value = pkt.value;

    ArenaBattleManager::GetInstance().RegisterPlayerBet(playerName, slot);
    IPCManager::GetInstance().SendLog(
        "[아레나] " + playerName + " 아이템 등록: " +
        std::string(pkt.itemName) + " (x" + std::to_string(pkt.amount) + ")");
}

void NetworkManager::OnHostArenaLobbyArrived(SOCKET sock)
{
    if (!Client::isServer) return;

    const std::string playerName = GetPlayerNameForSocket(sock);
    if (arenaLobbyArrived.find(playerName) != arenaLobbyArrived.end())
    {
        return;
    }

    arenaLobbyArrived.insert(playerName);

    const int arrived = GetArenaLobbyArrivedCount();
    const int expected = GetExpectedArenaPlayerCount();
    IPCManager::GetInstance().SendLog(
        "[아레나] " + playerName + " 로비 도착 (" +
        std::to_string(arrived) + "/" + std::to_string(expected) + ")");

    BroadcastArenaLobbyState();
}

void NetworkManager::OnHostArenaPlayerSnapshot(SOCKET sock, const char* packetData, size_t packetSize)
{
    if (packetData == nullptr || packetSize < sizeof(PacketHeader)) return;

    if (sock != INVALID_SOCKET && !arenaSnapshotCollecting)
    {
        IPCManager::GetInstance().SendLog(
            "[아레나] 스냅샷 수집 중이 아니어서 게스트 스냅샷 무시");
        return;
    }

    const auto* snapHdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(packetData);
    if (!IsValidArenaSnapshotSize(snapHdr->header.size, snapHdr->itemSlotCount)) return;
    if (snapHdr->header.size > packetSize) return;

    std::string playerName = GetPlayerNameForSocket(sock);
    if (sock == INVALID_SOCKET && snapHdr->playerName[0] != '\0')
    {
        playerName = snapHdr->playerName;
    }

    {
        std::vector<char> copy(packetData, packetData + snapHdr->header.size);
        arenaPlayerSnapshots[playerName] = std::move(copy);
    }

    IPCManager::GetInstance().SendLog(
        "[아레나] 스냅샷 수신: " + playerName +
        " HP:" + std::to_string(snapHdr->hp) +
        " 슬롯:" + std::to_string(snapHdr->itemSlotCount));

    const bool wasCollecting = arenaSnapshotCollecting;
    TryStartArenaBattleAfterSnapshots();

    if (wasCollecting && arenaBattleStarted)
    {
        arenaSnapshotCollecting = false;
    }
}

void NetworkManager::OnHostArenaAttack(SOCKET sock, const Pkt_ArenaAttack& pkt)
{
    std::string attackerName = GetPlayerNameForSocket(sock);
    std::string targetName = pkt.targetName;

    if (!IsActorsArenaTurn(attackerName))
    {
        IPCManager::GetInstance().SendLog("[아레나] 공격 실패: 현재 턴이 아닙니다.");
        return;
    }

    if (attackerName == targetName)
    {
        IPCManager::GetInstance().SendLog("[아레나] 공격 실패: 자신을 공격할 수 없습니다.");
        return;
    }

    if (!IsArenaPlayerAlive(targetName))
    {
        IPCManager::GetInstance().SendLog("[아레나] 공격 실패: 대상이 이미 탈락했습니다.");
        return;
    }

    auto attackerIt = arenaPlayerSnapshots.find(attackerName);
    auto targetIt = arenaPlayerSnapshots.find(targetName);
    if (attackerIt == arenaPlayerSnapshots.end() || targetIt == arenaPlayerSnapshots.end())
    {
        IPCManager::GetInstance().SendLog("[아레나] 공격 실패: 스냅샷 없음");
        return;
    }

    auto* attackerHdr = reinterpret_cast<Pkt_ArenaPlayerSnapshotHeader*>(attackerIt->second.data());
    auto* targetHdr = reinterpret_cast<Pkt_ArenaPlayerSnapshotHeader*>(targetIt->second.data());

    int32_t damage = attackerHdr->attack;
    if (damage < 0) damage = 0;
    targetHdr->hp -= damage;
    if (targetHdr->hp < 0) targetHdr->hp = 0;

    Pkt_ArenaAttackResult resultPkt;
    CopyStringToPacketField(resultPkt.attackerName, sizeof(resultPkt.attackerName), attackerName);
    CopyStringToPacketField(resultPkt.targetName, sizeof(resultPkt.targetName), targetName);
    resultPkt.damage = damage;
    BroadcastArenaAttackResult(resultPkt);

    Pkt_ArenaHpSync hpPkt;
    CopyStringToPacketField(hpPkt.playerName, sizeof(hpPkt.playerName), targetName);
    hpPkt.currentHp = targetHdr->hp;
    hpPkt.maxHp = targetHdr->maxHp;
    BroadcastArenaHpSync(hpPkt);

    if (targetHdr->hp <= 0)
    {
        BroadcastArenaDie(targetName);
        SendStateChangeToPlayer(targetName, EGameState::ArenaWait);
    }

    IPCManager::GetInstance().SendLog(
        "[아레나] 공격: " + attackerName + " -> " + targetName + " (-" + std::to_string(damage) + ")");

    TryEndArenaBattleIfNeeded();
    if (!arenaBattleStarted) return;

    AdvanceArenaTurnAfterAction();
}

void NetworkManager::TryEndArenaBattleIfNeeded()
{
    if (!Client::isServer || !arenaBattleStarted) return;

    ArenaBattleManager& arena = ArenaBattleManager::GetInstance();
    if (arena.IsBattleEnded()) return;

    const size_t totalPlayers = arena.GetSpectatorPlayers().size();
    if (totalPlayers < 2) return;

    const int aliveCount = arena.GetAliveCount();
    if (aliveCount > 1) return;

    Pkt_ArenaRankList rankPkt;
    if (!arena.TryBuildRankList(rankPkt))
    {
        IPCManager::GetInstance().SendLog("[아레나] 순위 생성 실패");
        return;
    }

    BroadcastArenaRankList(rankPkt);
    BroadcastArenaRewardPool();
    SendArenaSessionApplyToAllPlayers(rankPkt);

    arenaBattleStarted = false;
    arenaCurrentTurnPlayer.clear();
    arenaTurnOrder.clear();
    arenaTurnIndex = 0;

    IPCManager::GetInstance().SendLog("[아레나] 전투 종료 - 결과 화면으로 전환합니다.");
    ApplySyncedStateChange(EGameState::ArenaResult);
}

void NetworkManager::OnHostArenaItemUse(SOCKET sock, const Pkt_ArenaItemUse& pkt)
{
    std::string userName = GetPlayerNameForSocket(sock);
    std::string targetName = userName;

    if (!IsActorsArenaTurn(userName))
    {
        IPCManager::GetInstance().SendLog("[아레나] 아이템 사용 실패: 현재 턴이 아닙니다.");
        return;
    }

    auto userIt = arenaPlayerSnapshots.find(userName);
    if (userIt == arenaPlayerSnapshots.end())
    {
        IPCManager::GetInstance().SendLog("[아레나] 아이템 사용 실패: 스냅샷 없음");
        return;
    }

    auto* userHdr = reinterpret_cast<Pkt_ArenaPlayerSnapshotHeader*>(userIt->second.data());
    ArenaItemSlot* slots = reinterpret_cast<ArenaItemSlot*>(
        const_cast<char*>(userIt->second.data()) + ArenaSnapshotHeaderSize());

    int slotIndex = -1;

    for (uint8_t i = 0; i < userHdr->itemSlotCount && i < MAX_ARENA_ITEM_SLOTS; ++i)
    {
        if (std::string(slots[i].itemName) != pkt.itemName) continue;

        if (slots[i].count <= 0)
        {
            IPCManager::GetInstance().SendLog("[아레나] 아이템 사용 실패: 개수 없음");
            return;
        }

        slotIndex = static_cast<int>(i);
        break;
    }

    if (slotIndex < 0)
    {
        IPCManager::GetInstance().SendLog("[아레나] 아이템 사용 실패: 보유하지 않음");
        return;
    }

    ArenaItemSlot& usedSlot = slots[static_cast<size_t>(slotIndex)];
    const ItemType itemType = static_cast<ItemType>(usedSlot.itemType);
    const int32_t effectValue = usedSlot.value;
    
    auto targetIt = arenaPlayerSnapshots.find(targetName);
    if (targetIt == arenaPlayerSnapshots.end()) return;

    auto* targetHdr = reinterpret_cast<Pkt_ArenaPlayerSnapshotHeader*>(targetIt->second.data());

    Pkt_ArenaItemResult rptk;
    CopyStringToPacketField(rptk.userName, sizeof(rptk.userName), userName);
    CopyStringToPacketField(rptk.itemName, sizeof(rptk.itemName), pkt.itemName);
    rptk.value = effectValue;
    rptk.itemType = usedSlot.itemType;
    BroadcastArenaItemResult(rptk);

    switch (itemType)
    {
    case ItemType::HP_POTION:
    {
        targetHdr->hp += effectValue;
        if (targetHdr->hp > targetHdr->maxHp) targetHdr->hp = targetHdr->maxHp;

        Pkt_ArenaHpSync hpPkt;
        CopyStringToPacketField(hpPkt.playerName, sizeof(hpPkt.playerName), targetName);
        hpPkt.currentHp = targetHdr->hp;
        hpPkt.maxHp = targetHdr->maxHp;
        BroadcastArenaHpSync(hpPkt);

        IPCManager::GetInstance().SendLog(
            "[아레나] " + userName + " HP 회복 (" + std::string(pkt.itemName) +
            ", +" + std::to_string(effectValue) + ")");
        break;
    }
    case ItemType::ATTACK_BUFF:
    {
        targetHdr->attack += effectValue;

        BroadcastArenaPlayerListFromSnapshots();

        IPCManager::GetInstance().SendLog(
            "[아레나] " + userName + " 공격력 증가 (" + std::string(pkt.itemName) +
            ", +" + std::to_string(effectValue) + ", ATK " + std::to_string(targetHdr->attack) + ")");
        break;
    }
    default:
        IPCManager::GetInstance().SendLog("[아레나] 아이템 사용 실패: 사용할 수 없는 아이템");
        return;
    }

    --usedSlot.count;

    Pkt_ArenaItemList itemPkt;
    if (BuildArenaItemListPacket(userName, itemPkt))
    {
        BroadcastArenaItemList(itemPkt);
    }

    TryEndArenaBattleIfNeeded();
    if (!arenaBattleStarted) return;

    AdvanceArenaTurnAfterAction();
}

void NetworkManager::TryStartArenaBattleAfterSnapshots()
{
    if (arenaBattleStarted || !HasAllArenaSnapshots()) return;

    Pkt_ArenaPlayerList listPkt;
    BuildArenaPlayerListPacket(listPkt);

    arenaBattleStarted = true;
    arenaSnapshotCollecting = false;
    ResetArenaLobbyArrivalTracking();

    // 게스트는 ArenaBattleState일 때만 NotifyArenaBattle*가 동작하므로
    // CHANGE_STATE를 먼저 보낸 뒤 PlayerList/TurnStart/ItemList를 브로드캐스트한다.
    BuildArenaTurnOrder();
    SaveArenaInitialSnapshots();

    ApplySyncedStateChange(EGameState::ArenaBattle);

    BroadcastArenaPlayerList(listPkt);
    NotifyArenaBattlePlayerList(listPkt);

    StartFirstArenaTurn();
}

void NetworkManager::ApplySyncedStateChange(EGameState stateType)
{
    if (!Client::isServer) return;

    if (stateType == EGameState::ArenaReady)
    {
        ResetArenaLobbyArrivalTracking();
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

bool NetworkManager::RequestAllArenaPlayerSnapshots(Player* hostPlayer)
{
    if (!Client::isServer) return false;

    if (arenaSnapshotCollecting)
    {
        IPCManager::GetInstance().SendLog("[아레나] 이미 스냅샷 수집 중입니다.");
        return false;
    }

    if (!HasAllPlayersInArenaLobby())
    {
        IPCManager::GetInstance().SendLog("[아레나] 전원 로비 도착 전입니다.");
        return false;
    }

    if (hostPlayer == nullptr)
    {
        IPCManager::GetInstance().SendLog("[아레나] 호스트 플레이어 없음");
        return false;
    }

    arenaSnapshotCollecting = true;
    arenaPlayerSnapshots.clear();

    SendArenaPlayerSnapshotPacket(hostPlayer);
    BroadcastArenaSnapshotRequest();

    IPCManager::GetInstance().SendLog("[아레나] 스냅샷 수집 시작 (호스트 전송 + 게스트 요청)");
    return true;
}

void NetworkManager::BroadcastArenaSnapshotRequest()
{
    if (!Client::isServer) return;

    Pkt_ArenaSnapshotRequest pkt;
    BroadcastToClients(&pkt, pkt.header.size);
}

void NetworkManager::SendArenaLobbyArrivedPacket()
{
    if (Client::isServer)
    {
        OnHostArenaLobbyArrived(INVALID_SOCKET);
    }
    else
    {
        Pkt_ArenaReady pkt;
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::SendArenaPlayerSnapshotPacket(Player* player)
{
    std::vector<char> buffer = BuildArenaPlayerSnapshotPacket(player);
    if (buffer.empty()) return;

    if (Client::isServer)
    {
        OnHostArenaPlayerSnapshot(INVALID_SOCKET, buffer.data(), buffer.size());
    }
    else
    {
        SendToServer(buffer.data(), buffer.size());
    }
}

void NetworkManager::SendArenaAttackPacket(const std::string& targetName)
{
    Pkt_ArenaAttack pkt;
    CopyStringToPacketField(pkt.targetName, sizeof(pkt.targetName), targetName);

    if (Client::isServer)
    {
        OnHostArenaAttack(INVALID_SOCKET, pkt);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::SendArenaItemUsePacket(const std::string& itemName, const std::string& targetName)
{
    Pkt_ArenaItemUse pkt;
    CopyStringToPacketField(pkt.itemName, sizeof(pkt.itemName), itemName);
    CopyStringToPacketField(pkt.targetName, sizeof(pkt.targetName), targetName);

    if (Client::isServer)
    {
        OnHostArenaItemUse(INVALID_SOCKET, pkt);
    }
    else
    {
        SendToServer(&pkt, pkt.header.size);
    }
}

void NetworkManager::NotifyArenaBattlePlayerList(const Pkt_ArenaPlayerList& pkt)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle == nullptr) return;

    std::vector<ArenaPlayerListEntry> entries;
    entries.reserve(pkt.playerCount);
    for (uint8_t i = 0; i < pkt.playerCount && i < MAX_ARENA_PLAYERS; ++i)
    {
        entries.push_back(pkt.players[i]);
    }
    battle->OnPlayerList(entries);
}

void NetworkManager::NotifyArenaBattleTurnStart(const std::string& turnPlayerName)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
    {
        battle->OnTurnStart(turnPlayerName);
    }
}

void NetworkManager::NotifyArenaBattleAttackResult(const Pkt_ArenaAttackResult& pkt)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
        battle->OnAttackResult(pkt.attackerName, pkt.targetName, pkt.damage);
    ArenaWaitState* wait = GetArenaWaitState();
    if (wait != nullptr)
        wait->OnAttackResult(pkt.attackerName, pkt.targetName, pkt.damage);
}

void NetworkManager::NotifyArenaBattleHpSync(const Pkt_ArenaHpSync& pkt)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
    {
        battle->OnHpSync(pkt.playerName, pkt.currentHp, pkt.maxHp);
    }
}

void NetworkManager::NotifyArenaBattleItemList(const Pkt_ArenaItemList& pkt)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle == nullptr) return;

    if (std::string(pkt.ownerName) != Client::playerName) return;

    std::vector<ArenaItemSlot> items;
    for (uint8_t i = 0; i < pkt.slotCount && i < MAX_ARENA_ITEM_SLOTS; ++i)
    {
        items.push_back(pkt.slots[i]);
    }
    battle->OnItemList(items);
}

void NetworkManager::NotifyArenaBattleDie(const std::string& playerName)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
    {
        battle->OnPlayerDie(playerName);
    }
}

void NetworkManager::NotifyArenaItemResult(const Pkt_ArenaItemResult& pkt)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
        battle->OnItemResult(pkt.userName, pkt.itemName, pkt.itemType, pkt.value);

    ArenaWaitState* wait = GetArenaWaitState();
    if (wait != nullptr)
        wait->OnItemResult(pkt.userName, pkt.itemName, pkt.itemType, pkt.value);
}

void NetworkManager::BroadcastArenaPlayerList(const Pkt_ArenaPlayerList& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorPlayerList(pkt);
        NotifyArenaBattlePlayerList(pkt);
    }
}

void NetworkManager::BroadcastArenaTurnStart(const std::string& turnPlayerName)
{
    if (!Client::isServer) return;

    Pkt_ArenaTurnStart pkt;
    CopyStringToPacketField(pkt.turnPlayerName, sizeof(pkt.turnPlayerName), turnPlayerName);
    BroadcastToClients(&pkt, pkt.header.size);
    ArenaBattleManager::GetInstance().OnSpectatorTurnStart(turnPlayerName);
    NotifyArenaBattleTurnStart(turnPlayerName);
}

void NetworkManager::BroadcastArenaAttackResult(const Pkt_ArenaAttackResult& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorAttackResult(pkt);
        NotifyArenaBattleAttackResult(pkt);
    }
}

void NetworkManager::BroadcastArenaHpSync(const Pkt_ArenaHpSync& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorHpSync(pkt);
        NotifyArenaBattleHpSync(pkt);
    }
}

void NetworkManager::BroadcastArenaItemList(const Pkt_ArenaItemList& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        NotifyArenaBattleItemList(pkt);
    }
}

void NetworkManager::BroadcastArenaDie(const std::string& playerName)
{
    if (!Client::isServer) return;

    Pkt_ArenaDie pkt;
    CopyStringToPacketField(pkt.playerName, sizeof(pkt.playerName), playerName);
    BroadcastToClients(&pkt, pkt.header.size);
    ArenaBattleManager::GetInstance().OnSpectatorDie(playerName);
    NotifyArenaBattleDie(playerName);
}

void NetworkManager::BroadcastArenaRankList(const Pkt_ArenaRankList& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorRankList(pkt);
    }
}

void NetworkManager::BroadcastArenaLobbyState()
{
    if (!Client::isServer) return;

    Pkt_ArenaLobbyState pkt;
    BuildArenaLobbyStatePacket(pkt);
    ApplyArenaLobbyStateCache(pkt);
    BroadcastToClients(&pkt, pkt.header.size);
}

void NetworkManager::BroadcastArenaItemResult(const Pkt_ArenaItemResult& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if(Client::isServer)
    {
        NotifyArenaItemResult(pkt);
    }
}

void NetworkManager::ApplyArenaLobbyStateCache(const Pkt_ArenaLobbyState& pkt)
{
    //UI용 캐시 초기화
    arenaLobbyDisplay.clear();
    for (uint8_t i = 0; i < pkt.playerCount && i < MAX_ARENA_PLAYERS; ++i)
    {
        // 플레이어 ArenaLobbyPlayerEntry로 저장
        arenaLobbyDisplay.push_back(pkt.players[i]);
    }
}

void NetworkManager::BuildArenaLobbyStatePacket(Pkt_ArenaLobbyState& out) const
{
    out = Pkt_ArenaLobbyState();

    //현재 있어야하는 플레이어 목록
    const std::set<std::string> expected = CollectExpectedArenaPlayerNames();
    uint8_t idx = 0;
    for (const std::string& name : expected)
    {
        if (idx >= MAX_ARENA_PLAYERS) break;

        CopyStringToPacketField(
            out.players[idx].playerName,
            sizeof(out.players[idx].playerName),
            name);

        // 접속 중인지 확인
        out.players[idx].hasArrived =
            (arenaLobbyArrived.find(name) != arenaLobbyArrived.end()) ? 1 : 0;

        // 플레이어 카운트
        ++idx;
    }
    out.playerCount = idx;
}




#pragma endregion

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

#pragma region Arena Packet Sending

namespace {
    void BuildArenaBetRefundPacket(const std::vector<ArenaItemSlot>& bets, Pkt_ArenaBetRefund& out)
    {
        out = Pkt_ArenaBetRefund();
        uint8_t idx = 0;
        for (const ArenaItemSlot& slot : bets)
        {
            if (idx >= MAX_ARENA_ITEM_SLOTS) break;
            if (slot.count <= 0) continue;
            out.slots[idx++] = slot;
        }
        out.slotCount = idx;
        out.header.size = static_cast<uint16_t>(
            offsetof(Pkt_ArenaBetRefund, slots) + idx * sizeof(ArenaItemSlot));
    }
}

void NetworkManager::SendArenaItemRegisterPacket(const std::string& itemName, int count,
    ItemType itemType, int32_t value)
{
    Pkt_ArenaItemRegister pkt;
    CopyStringToPacketField(pkt.itemName, sizeof(pkt.itemName), itemName);
    pkt.amount = count;
    pkt.itemType = static_cast<uint8_t>(itemType);
    pkt.value = value;

    if (Client::isServer)
    {
        OnHostArenaItemRegister(INVALID_SOCKET, pkt);
        IPCManager::GetInstance().SendLog(
            "[아레나] 호스트 아이템 등록됨: " + itemName + " (x" + std::to_string(count) + ")");
    }
    else if (clientSocket != INVALID_SOCKET)
    {
        send(clientSocket, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);
    }
}

void NetworkManager::CancelArenaPreparation()
{
    if (!Client::isServer) return;

    ArenaBattleManager& arena = ArenaBattleManager::GetInstance();
    const std::map<std::string, std::vector<ArenaItemSlot>>& betsByPlayer = arena.GetBetsByPlayer();

    for (const auto& pair : betsByPlayer)
    {
        Pkt_ArenaBetRefund refundPkt;
        BuildArenaBetRefundPacket(pair.second, refundPkt);

        if (pair.first == Client::playerName)
        {
            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                ApplyArenaBetRefundToLocalPlayer(player, refundPkt);
            }
        }
        else
        {
            SendToPlayerByName(pair.first, &refundPkt, refundPkt.header.size);
        }
    }

    arena.ClearAllArenaBets();
    ResetArenaLobbyArrivalTracking();

    IPCManager::GetInstance().SendLog("\033[1;34m방장이 아레나를 취소했습니다. 베팅 아이템을 반환합니다.\033[0m");
    ApplySyncedStateChange(EGameState::Start);
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
    ClearArenaSessionData();

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

                    if (GetArenaLobbyState() != nullptr && !arenaBattleStarted)
                    {
                        BroadcastArenaLobbyState();
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
