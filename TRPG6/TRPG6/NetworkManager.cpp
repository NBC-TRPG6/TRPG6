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
#include "IGameState.h"
#include "DATABASE.h"
#include "ArenaBattleState.h"
#include "TradeManager.h"
#include <algorithm>
#include <set>

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

        // ---------- 아레나 S2C (게스트: ArenaBattleManager 관전 캐시 갱신) ----------
        case PacketType::PKT_S2C_ARENA_PLAYER_LIST: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaPlayerList*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorPlayerList(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] 플레이어 목록 수신 (" + std::to_string(pkt->playerCount) + "명)");
            break;
        }

        case PacketType::PKT_S2C_ARENA_TURN_START: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaTurnStart*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorTurnStart(pkt->turnPlayerName);
            IPCManager::GetInstance().SendLog(
                "[아레나] 턴 시작: " + std::string(pkt->turnPlayerName));
            break;
        }

        case PacketType::PKT_S2C_ARENA_ATTACK_RESULT: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaAttackResult*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorAttackResult(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] " + std::string(pkt->attackerName) + " -> " +
                std::string(pkt->targetName) + " 데미지 " + std::to_string(pkt->damage));
            break;
        }

        case PacketType::PKT_S2C_ARENA_HP_SYNC: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaHpSync*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorHpSync(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] HP: " + std::string(pkt->playerName) +
                " " + std::to_string(pkt->currentHp) + "/" + std::to_string(pkt->maxHp));
            break;
        }

        case PacketType::PKT_S2C_ARENA_ITEM_LIST: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaItemList*>(header);
            IPCManager::GetInstance().SendLog(
                "[아레나] 아이템 목록: " + std::string(pkt->ownerName) +
                " (" + std::to_string(pkt->slotCount) + "종)");
            break;
        }

        case PacketType::PKT_S2C_ARENA_DIE: {
            if (Client::isServer) break;
            auto* pkt = reinterpret_cast<Pkt_ArenaDie*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorDie(pkt->playerName);
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

        case PacketType::PKT_C2S_TRADE_REQUEST:
        {
            Pkt_TradeRequest * pkt = reinterpret_cast<Pkt_TradeRequest*>(header);
            // 방장만 이 패킷을 처리해서 ID를 부여함
            if (Client::isServer)
            {
                TradeManager::GetInstance().Server_HandleRequest(pkt->info);
            }
            break;
        }

        case PacketType::PKT_C2S_TRADE_RESPONSE:
        {
            Pkt_TradeResponse * pkt = reinterpret_cast<Pkt_TradeResponse*>(header);
            // 방장만 이 패킷을 받아서 최종 수락/거절 판정을 내림
            if (Client::isServer)
            {
                TradeManager::GetInstance().Server_HandleResponse(pkt->tradeId, pkt->response);
            }
            break;
        }

        case PacketType::PKT_S2C_TRADE_SYNC:
        {
            Pkt_TradeSync * pkt = reinterpret_cast<Pkt_TradeSync*>(header);
            // 서버가 갱신된 리스트를 보내주면 모두가 내 로컬 리스트를 동기화함
            // (이 과정에서 상태가 1(성공)로 변했다면 아이템 교환 로직도 같이 실행됨)
            TradeManager::GetInstance().SyncTrade(pkt->info);
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
    arenaBattleStarted = false;
    ResetArenaLobbyArrivalTracking();
}

void NetworkManager::ResetArenaLobbyArrivalTracking()
{
    arenaLobbyArrived.clear();
    arenaAllLobbyConfirmed = false;
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
    return static_cast<int>(CollectExpectedArenaPlayerNames().size());
}

int NetworkManager::GetArenaLobbyArrivedCount() const
{
    return static_cast<int>(arenaLobbyArrived.size());
}

bool NetworkManager::HasAllPlayersInArenaLobby() const
{
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
    ArenaBattleManager::GetInstance().AddBettedItem(pkt.itemName, pkt.amount);
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

    TryConfirmAllPlayersInArenaLobby();
}

void NetworkManager::TryConfirmAllPlayersInArenaLobby()
{
    if (!Client::isServer || arenaAllLobbyConfirmed) return;
    if (!HasAllPlayersInArenaLobby()) return;

    arenaAllLobbyConfirmed = true;
    IPCManager::GetInstance().SendLog("[아레나] 전원 로비 도착 완료. 방장이 전투를 시작할 수 있습니다.");
}

void NetworkManager::OnHostArenaPlayerSnapshot(SOCKET sock, const char* packetData, size_t packetSize)
{
    if (packetData == nullptr || packetSize < sizeof(PacketHeader)) return;

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

    TryStartArenaBattleAfterSnapshots();
}

// 스냅샷 기준 데미지 계산 -> AttackResult/HpSync/Die 브로드캐스트, 탈락자는 ArenaWait
void NetworkManager::OnHostArenaAttack(SOCKET sock, const Pkt_ArenaAttack& pkt)
{
    std::string attackerName = GetPlayerNameForSocket(sock);
    std::string targetName = pkt.targetName;

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

    // 생존 1명 이하이면 순위 브로드캐스트 후 전원 ArenaResult
    TryEndArenaBattleIfNeeded();

    IPCManager::GetInstance().SendLog(
        "[아레나] 공격: " + attackerName + " -> " + targetName + " (-" + std::to_string(damage) + ")");
}

// 참가 2명 이상·생존 1명 이하일 때 RankList 전송 + ApplySyncedStateChange(ArenaResult)
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
    arenaBattleStarted = false;

    IPCManager::GetInstance().SendLog("[아레나] 전투 종료 - 결과 화면으로 전환합니다.");
    ApplySyncedStateChange(EGameState::ArenaResult);
}

void NetworkManager::OnHostArenaItemUse(SOCKET sock, const Pkt_ArenaItemUse& pkt)
{
    std::string userName = GetPlayerNameForSocket(sock);
    std::string targetName = pkt.targetName[0] != '\0' ? std::string(pkt.targetName) : userName;

    auto userIt = arenaPlayerSnapshots.find(userName);
    if (userIt == arenaPlayerSnapshots.end())
    {
        IPCManager::GetInstance().SendLog("[아레나] 아이템 사용 실패: 스냅샷 없음");
        return;
    }

    auto* userHdr = reinterpret_cast<Pkt_ArenaPlayerSnapshotHeader*>(userIt->second.data());
    const ArenaItemSlot* slots = GetArenaSnapshotItems(userHdr);
    int32_t healAmount = 0;

    for (uint8_t i = 0; i < userHdr->itemSlotCount; ++i)
    {
        if (std::string(slots[i].itemName) == pkt.itemName)
        {
            healAmount = slots[i].value;
            break;
        }
    }

    auto targetIt = arenaPlayerSnapshots.find(targetName);
    if (targetIt == arenaPlayerSnapshots.end()) return;

    auto* targetHdr = reinterpret_cast<Pkt_ArenaPlayerSnapshotHeader*>(targetIt->second.data());
    targetHdr->hp += healAmount;
    if (targetHdr->hp > targetHdr->maxHp) targetHdr->hp = targetHdr->maxHp;

    Pkt_ArenaHpSync hpPkt;
    CopyStringToPacketField(hpPkt.playerName, sizeof(hpPkt.playerName), targetName);
    hpPkt.currentHp = targetHdr->hp;
    hpPkt.maxHp = targetHdr->maxHp;
    BroadcastArenaHpSync(hpPkt);

    IPCManager::GetInstance().SendLog(
        "[아레나] 아이템 사용: " + userName + " / " + std::string(pkt.itemName));
}

// 전원 스냅샷 수집 완료 시 PlayerList/TurnStart/ItemList 후 ArenaBattle 시작
void NetworkManager::TryStartArenaBattleAfterSnapshots()
{
    if (arenaBattleStarted || !HasAllArenaSnapshots()) return;

    Pkt_ArenaPlayerList listPkt;
    uint8_t idx = 0;

    for (const auto& pair : arenaPlayerSnapshots)
    {
        if (idx >= MAX_ARENA_PLAYERS) break;

        const auto* hdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(pair.second.data());
        CopyStringToPacketField(listPkt.players[idx].playerName, sizeof(listPkt.players[idx].playerName), pair.first);
        listPkt.players[idx].hp = hdr->hp;
        listPkt.players[idx].maxHp = hdr->maxHp;
        listPkt.players[idx].attack = hdr->attack;
        listPkt.players[idx].level = hdr->level;
        listPkt.players[idx].isAlive = hdr->hp > 0 ? 1 : 0;
        ++idx;
    }
    listPkt.playerCount = idx;

    BroadcastArenaPlayerList(listPkt);

    if (!arenaPlayerSnapshots.empty())
    {
        const auto& firstPair = arenaPlayerSnapshots.begin();
        BroadcastArenaTurnStart(firstPair->first);

        const auto* firstHdr = reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(firstPair->second.data());
        const ArenaItemSlot* slots = GetArenaSnapshotItems(firstHdr);

        Pkt_ArenaItemList itemPkt;
        CopyStringToPacketField(itemPkt.ownerName, sizeof(itemPkt.ownerName), firstPair->first);
        itemPkt.slotCount = firstHdr->itemSlotCount;
        if (itemPkt.slotCount > MAX_ARENA_ITEM_SLOTS) itemPkt.slotCount = MAX_ARENA_ITEM_SLOTS;

        for (uint8_t i = 0; i < itemPkt.slotCount; ++i)
        {
            itemPkt.slots[i] = slots[i];
        }
        itemPkt.header.size = static_cast<uint16_t>(
            offsetof(Pkt_ArenaItemList, slots) + itemPkt.slotCount * sizeof(ArenaItemSlot));
        BroadcastArenaItemList(itemPkt);
    }

    arenaBattleStarted = true;
    ResetArenaLobbyArrivalTracking();
    ApplySyncedStateChange(EGameState::ArenaBattle);
}

// 호스트 로컬 상태 전환 + 모든 게스트에 PKT_S2C_CHANGE_STATE
void NetworkManager::ApplySyncedStateChange(EGameState stateType)
{
    if (!Client::isServer) return;

    if (stateType == EGameState::ArenaReady)
    {
        ResetArenaLobbyArrivalTracking();
    }

    IGameState* nextState = CreateStateFromEGameState(stateType);
    if (nextState != nullptr)
    {
        GameManager::GetInstance().SetCurrentState(nextState);
    }

    BroadcastChangeState(stateType);
}

// 한 명만 상태 변경(탈락 ArenaWait 등). 호스트 본인은 SetCurrentState, 게스트는 CHANGE_STATE 전송
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

// C2S: 로컬은 이미 ArenaLobby, 서버에 도착만 알림(상태 전환 없음)
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

// C2S 가변 스냅샷: BuildArenaPlayerSnapshotPacket 후 호스트/게스트 분기
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

// 게스트에 전송 + 호스트 로컬 관전 캐시 갱신(방장은 S2C를 직접 받지 않음)
void NetworkManager::BroadcastArenaPlayerList(const Pkt_ArenaPlayerList& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorPlayerList(pkt);
    }
}

void NetworkManager::BroadcastArenaTurnStart(const std::string& turnPlayerName)
{
    if (!Client::isServer) return;

    Pkt_ArenaTurnStart pkt;
    CopyStringToPacketField(pkt.turnPlayerName, sizeof(pkt.turnPlayerName), turnPlayerName);
    BroadcastToClients(&pkt, pkt.header.size);
    ArenaBattleManager::GetInstance().OnSpectatorTurnStart(turnPlayerName);
}

// 공격 결과 브로드캐스트 + 호스트 관전 로그 갱신
void NetworkManager::BroadcastArenaAttackResult(const Pkt_ArenaAttackResult& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorAttackResult(pkt);
    }
}

void NetworkManager::BroadcastArenaHpSync(const Pkt_ArenaHpSync& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorHpSync(pkt);
    }
}

// 턴 보유자 인벤 목록(관전 UI에는 미사용, 전투 측 연동용)
void NetworkManager::BroadcastArenaItemList(const Pkt_ArenaItemList& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
}

// 사망 통지 + 탈락 순서·관전 캐시 갱신
void NetworkManager::BroadcastArenaDie(const std::string& playerName)
{
    if (!Client::isServer) return;

    Pkt_ArenaDie pkt;
    CopyStringToPacketField(pkt.playerName, sizeof(pkt.playerName), playerName);
    BroadcastToClients(&pkt, pkt.header.size);
    ArenaBattleManager::GetInstance().OnSpectatorDie(playerName);
}

// 전투 종료 순위. 호스트·게스트 모두 battleEnded 및 rankEntries 갱신
void NetworkManager::BroadcastArenaRankList(const Pkt_ArenaRankList& pkt)
{
    BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorRankList(pkt);
    }
}

#pragma endregion

#pragma region Item Implementation
void NetworkManager::SendTradeRequest(const Pkt_TradeRequest & pkt)
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

void NetworkManager::SendTradeResponse(const Pkt_TradeResponse & pkt)
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

void NetworkManager::BroadcastTradeSync(const Pkt_TradeSync & pkt)
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

#pragma region Arena Packet Sending

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
