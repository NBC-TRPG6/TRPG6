#include "ArenaNetworkManager.h"
#include "NetworkManager.h"
#include "IPCManager.h"
#include "GameManager.h"
#include "ArenaReadyState.h"
#include "ArenaLobbyState.h"
#include "ArenaBattleState.h"
#include "ArenaWaitState.h"
#include "ArenaBattleManager.h"
#include "Player.h"
#include "Item.h"
#include "IGameState.h"
#include "DATABASE.h"
#include <algorithm>
#include <set>
#include <cstring>

#pragma region Local Helpers

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

#pragma endregion

#pragma region Packet Dispatch

bool ArenaNetworkManager::TryHandlePacket(SOCKET sock, PacketHeader* header)
{
    switch (header->type)
    {
    case PacketType::PKT_C2S_ARENA_ITEM_REGISTER:
        if (!Client::isServer) return true;
        OnHostArenaItemRegister(sock, *reinterpret_cast<Pkt_ArenaItemRegister*>(header));
        return true;

    case PacketType::PKT_C2S_ARENA_READY:
        if (!Client::isServer) return true;
        OnHostArenaLobbyArrived(sock);
        return true;

    case PacketType::PKT_C2S_ARENA_PLAYER_SNAPSHOT:
        if (!Client::isServer) return true;
        OnHostArenaPlayerSnapshot(sock, reinterpret_cast<const char*>(header), header->size);
        return true;

    case PacketType::PKT_C2S_ARENA_ATTACK:
        if (!Client::isServer) return true;
        OnHostArenaAttack(sock, *reinterpret_cast<Pkt_ArenaAttack*>(header));
        return true;

    case PacketType::PKT_C2S_ARENA_ITEM_USE:
        if (!Client::isServer) return true;
        OnHostArenaItemUse(sock, *reinterpret_cast<Pkt_ArenaItemUse*>(header));
        return true;

    case PacketType::PKT_S2C_ARENA_PLAYER_LIST:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaPlayerList*>(header);
            NotifyArenaBattlePlayerList(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] 플레이어 목록 수신 (" + std::to_string(pkt->playerCount) + "명)");
        }
        return true;

    case PacketType::PKT_S2C_ARENA_TURN_START:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaTurnStart*>(header);
            NotifyArenaBattleTurnStart(pkt->turnPlayerName);
            IPCManager::GetInstance().SendLog(
                "[아레나] 턴 시작: " + std::string(pkt->turnPlayerName));
        }
        return true;

    case PacketType::PKT_S2C_ARENA_ATTACK_RESULT:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaAttackResult*>(header);
            NotifyArenaBattleAttackResult(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] " + std::string(pkt->attackerName) + " -> " +
                std::string(pkt->targetName) + " 데미지 " + std::to_string(pkt->damage));
        }
        return true;

    case PacketType::PKT_S2C_ARENA_ITEM_RESULT:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaItemResult*>(header);
            NotifyArenaItemResult(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] " + std::string(pkt->userName) + " 사용 " +
                std::string(pkt->itemName) + " -> 효과: " + std::to_string(pkt->value));
        }
        return true;

    case PacketType::PKT_S2C_ARENA_HP_SYNC:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaHpSync*>(header);
            NotifyArenaBattleHpSync(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] HP: " + std::string(pkt->playerName) +
                " " + std::to_string(pkt->currentHp) + "/" + std::to_string(pkt->maxHp));
        }
        return true;

    case PacketType::PKT_S2C_ARENA_ITEM_LIST:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaItemList*>(header);
            NotifyArenaBattleItemList(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] 아이템 목록: " + std::string(pkt->ownerName) +
                " (" + std::to_string(pkt->slotCount) + "종)");
        }
        return true;

    case PacketType::PKT_S2C_ARENA_DIE:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaDie*>(header);
            NotifyArenaBattleDie(pkt->playerName);
            IPCManager::GetInstance().SendLog("[아레나] 사망: " + std::string(pkt->playerName));
        }
        return true;

    case PacketType::PKT_S2C_ARENA_RANK_LIST:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaRankList*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorRankList(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] 순위 수신 (" + std::to_string(pkt->entryCount) + "명)");
        }
        return true;

    case PacketType::PKT_S2C_ARENA_REWARD_POOL:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaRewardPool*>(header);
            ArenaBattleManager::GetInstance().OnSpectatorRewardPool(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] 보상 풀 수신 (" + std::to_string(pkt->slotCount) + "종)");
        }
        return true;

    case PacketType::PKT_S2C_ARENA_SESSION_APPLY:
        if (Client::isServer) return true;
        {
            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                ApplyArenaSessionToLocalPlayer(
                    player,
                    reinterpret_cast<const char*>(header),
                    header->size);
            }
        }
        return true;

    case PacketType::PKT_S2C_ARENA_LOBBY_STATE:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaLobbyState*>(header);
            ApplyArenaLobbyStateCache(*pkt);
            IPCManager::GetInstance().SendLog(
                "[아레나] 로비 상태 수신 (" + std::to_string(pkt->playerCount) + "명)");
        }
        return true;

    case PacketType::PKT_S2C_ARENA_BET_REFUND:
        if (Client::isServer) return true;
        {
            auto* pkt = reinterpret_cast<Pkt_ArenaBetRefund*>(header);
            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                ApplyArenaBetRefundToLocalPlayer(player, *pkt);
            }
        }
        return true;

    case PacketType::PKT_S2C_ARENA_SNAPSHOT_REQUEST:
        if (Client::isServer) return true;
        if (GetArenaLobbyState() == nullptr)
        {
            IPCManager::GetInstance().SendLog(
                "[아레나] 로비가 아닌 상태에서 스냅샷 요청 무시");
            return true;
        }
        {
            Player* player = GameManager::GetInstance().GetPlayer();
            if (player != nullptr)
            {
                SendArenaPlayerSnapshotPacket(player);
                IPCManager::GetInstance().SendLog("[아레나] 방장 요청으로 스냅샷 전송");
            }
        }
        return true;

    default:
        return false;
    }
}

void ArenaNetworkManager::OnPlayerDisconnected()
{
    if (!Client::isServer) return;
    if (GetArenaLobbyState() != nullptr && !arenaBattleStarted)
    {
        BroadcastArenaLobbyState();
    }
}

#pragma endregion

#pragma region Session State

void ArenaNetworkManager::ClearSessionData()
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

bool ArenaNetworkManager::IsArenaPlayerAlive(const std::string& playerName) const
{
    auto it = arenaPlayerSnapshots.find(playerName);
    if (it == arenaPlayerSnapshots.end()) return false;

    const Pkt_ArenaPlayerSnapshotHeader* hdr =
        reinterpret_cast<const Pkt_ArenaPlayerSnapshotHeader*>(it->second.data());
    return hdr->hp > 0;
}

bool ArenaNetworkManager::IsActorsArenaTurn(const std::string& actorName) const
{
    return arenaBattleStarted
        && !arenaCurrentTurnPlayer.empty()
        && actorName == arenaCurrentTurnPlayer
        && IsArenaPlayerAlive(actorName);
}

void ArenaNetworkManager::BuildArenaTurnOrder()
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

void ArenaNetworkManager::SaveArenaInitialSnapshots()
{
    arenaInitialPlayerSnapshots.clear();
    for (const auto& pair : arenaPlayerSnapshots)
    {
        arenaInitialPlayerSnapshots[pair.first] = pair.second;
    }
}

bool ArenaNetworkManager::HasAllArenaSnapshots()
{
    const std::set<std::string> expected = CollectExpectedArenaPlayerNames();

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

#pragma endregion

#pragma region Packet Builders

void ArenaNetworkManager::BuildArenaPlayerListPacket(Pkt_ArenaPlayerList& out) const
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

bool ArenaNetworkManager::BuildArenaItemListPacket(const std::string& ownerName, Pkt_ArenaItemList& out) const
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

int ArenaNetworkManager::GetArenaRankForPlayer(const Pkt_ArenaRankList& rankPkt, const std::string& playerName)
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

bool ArenaNetworkManager::BuildArenaSessionApplyPacket(const std::string& playerName,
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

void ArenaNetworkManager::BuildArenaRewardPoolPacket(Pkt_ArenaRewardPool& out) const
{
    out = Pkt_ArenaRewardPool();

    const auto bettedItems =
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

void ArenaNetworkManager::BuildArenaLobbyStatePacket(Pkt_ArenaLobbyState& out) const
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

#pragma region Lobby

void ArenaNetworkManager::ResetArenaLobbyArrivalTracking()
{
    arenaLobbyArrived.clear();
    arenaLobbyDisplay.clear();
}

std::set<std::string> ArenaNetworkManager::CollectExpectedArenaPlayerNames() const
{
    std::set<std::string> expected;
    expected.insert(Client::playerName);

    for (const std::string& name : NetworkManager::GetInstance().GetConnectedPlayerNames())
    {
        expected.insert(name);
    }
    return expected;
}

int ArenaNetworkManager::GetExpectedArenaPlayerCount() const
{
    if (!arenaLobbyDisplay.empty())
    {
        return static_cast<int>(arenaLobbyDisplay.size());
    }
    return static_cast<int>(CollectExpectedArenaPlayerNames().size());
}

int ArenaNetworkManager::GetArenaLobbyArrivedCount() const
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

bool ArenaNetworkManager::HasAllPlayersInArenaLobby() const
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

const std::vector<ArenaLobbyPlayerEntry>& ArenaNetworkManager::GetArenaLobbyPlayers() const
{
    return arenaLobbyDisplay;
}

// 호스트 이름 + clientNames 전원이 arenaPlayerSnapshots에 있는지 확인
void ArenaNetworkManager::ApplyArenaLobbyStateCache(const Pkt_ArenaLobbyState& pkt)
{
    //UI용 캐시 초기화
    arenaLobbyDisplay.clear();
    for (uint8_t i = 0; i < pkt.playerCount && i < MAX_ARENA_PLAYERS; ++i)
    {
        // 플레이어 ArenaLobbyPlayerEntry로 저장
        arenaLobbyDisplay.push_back(pkt.players[i]);
    }
}

#pragma endregion

#pragma region S2C Local Apply (Notify)

void ArenaNetworkManager::NotifyArenaBattlePlayerList(const Pkt_ArenaPlayerList& pkt)
{
    ArenaBattleManager::GetInstance().OnSpectatorPlayerList(pkt);

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

void ArenaNetworkManager::NotifyArenaBattleTurnStart(const std::string& turnPlayerName)
{
    ArenaBattleManager::GetInstance().OnSpectatorTurnStart(turnPlayerName);
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
    {
        battle->OnTurnStart(turnPlayerName);
    }
}

void ArenaNetworkManager::NotifyArenaBattleAttackResult(const Pkt_ArenaAttackResult& pkt)
{
    ArenaBattleManager::GetInstance().OnSpectatorAttackResult(pkt);
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
        battle->OnAttackResult(pkt.attackerName, pkt.targetName, pkt.damage);
    ArenaWaitState* wait = GetArenaWaitState();
    if (wait != nullptr)
        wait->OnAttackResult(pkt.attackerName, pkt.targetName, pkt.damage);
}

void ArenaNetworkManager::NotifyArenaBattleHpSync(const Pkt_ArenaHpSync& pkt)
{
    ArenaBattleManager::GetInstance().OnSpectatorHpSync(pkt);
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
    {
        battle->OnHpSync(pkt.playerName, pkt.currentHp, pkt.maxHp);
    }
}

void ArenaNetworkManager::NotifyArenaBattleItemList(const Pkt_ArenaItemList& pkt)
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

void ArenaNetworkManager::NotifyArenaBattleDie(const std::string& playerName)
{
    ArenaBattleManager::GetInstance().OnSpectatorDie(playerName);

    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
    {
        battle->OnPlayerDie(playerName);
    }
}

void ArenaNetworkManager::NotifyArenaItemResult(const Pkt_ArenaItemResult& pkt)
{
    ArenaBattleState* battle = GetArenaBattleState();
    if (battle != nullptr)
        battle->OnItemResult(pkt.userName, pkt.itemName, pkt.itemType, pkt.value);

    ArenaWaitState* wait = GetArenaWaitState();
    if (wait != nullptr)
        wait->OnItemResult(pkt.userName, pkt.itemName, pkt.itemType, pkt.value);
}

#pragma endregion

#pragma region S2C Broadcast

void ArenaNetworkManager::BroadcastArenaPlayerList(const Pkt_ArenaPlayerList& pkt)
{
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        NotifyArenaBattlePlayerList(pkt);
    }
}

void ArenaNetworkManager::BroadcastArenaTurnStart(const std::string& turnPlayerName)
{
    if (!Client::isServer) return;

    Pkt_ArenaTurnStart pkt;
    CopyStringToPacketField(pkt.turnPlayerName, sizeof(pkt.turnPlayerName), turnPlayerName);
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    NotifyArenaBattleTurnStart(turnPlayerName);
}

void ArenaNetworkManager::BroadcastArenaAttackResult(const Pkt_ArenaAttackResult& pkt)
{
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        NotifyArenaBattleAttackResult(pkt);
    }
}

void ArenaNetworkManager::BroadcastArenaHpSync(const Pkt_ArenaHpSync& pkt)
{
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        NotifyArenaBattleHpSync(pkt);
    }
}

void ArenaNetworkManager::BroadcastArenaItemList(const Pkt_ArenaItemList& pkt)
{
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        NotifyArenaBattleItemList(pkt);
    }
}

void ArenaNetworkManager::BroadcastArenaDie(const std::string& playerName)
{
    if (!Client::isServer) return;

    Pkt_ArenaDie pkt;
    CopyStringToPacketField(pkt.playerName, sizeof(pkt.playerName), playerName);
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    NotifyArenaBattleDie(playerName);
}

void ArenaNetworkManager::BroadcastArenaRankList(const Pkt_ArenaRankList& pkt)
{
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    if (Client::isServer)
    {
        ArenaBattleManager::GetInstance().OnSpectatorRankList(pkt);
    }
}

void ArenaNetworkManager::BroadcastArenaLobbyState()
{
    if (!Client::isServer) return;

    Pkt_ArenaLobbyState pkt;
    BuildArenaLobbyStatePacket(pkt);
    ApplyArenaLobbyStateCache(pkt);
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
}

void ArenaNetworkManager::BroadcastArenaItemResult(const Pkt_ArenaItemResult& pkt)
{
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
    if(Client::isServer)
    {
        NotifyArenaItemResult(pkt);
    }
}

void ArenaNetworkManager::BroadcastArenaPlayerListFromSnapshots()
{
    if (!Client::isServer) return;

    Pkt_ArenaPlayerList listPkt;
    BuildArenaPlayerListPacket(listPkt);
    BroadcastArenaPlayerList(listPkt);
}

void ArenaNetworkManager::BroadcastArenaTurnAndItems(const std::string& playerName)
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

void ArenaNetworkManager::BroadcastArenaRewardPool()
{
    if (!Client::isServer) return;

    Pkt_ArenaRewardPool pkt;
    BuildArenaRewardPoolPacket(pkt);

    ArenaBattleManager::GetInstance().OnSpectatorRewardPool(pkt);
    NetworkManager::GetInstance().BroadcastToClients(&pkt,pkt.header.size);
}

void ArenaNetworkManager::BroadcastArenaSnapshotRequest()
{
    if (!Client::isServer) return;

    Pkt_ArenaSnapshotRequest pkt;
    NetworkManager::GetInstance().BroadcastToClients(&pkt, pkt.header.size);
}

#pragma endregion

#pragma region Host C2S Receive

void ArenaNetworkManager::OnHostArenaItemRegister(SOCKET sock, const Pkt_ArenaItemRegister& pkt)
{
    std::string playerName = NetworkManager::GetInstance().GetPlayerNameForSocket(sock);

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

void ArenaNetworkManager::OnHostArenaLobbyArrived(SOCKET sock)
{
    if (!Client::isServer) return;

    const std::string playerName = NetworkManager::GetInstance().GetPlayerNameForSocket(sock);
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

void ArenaNetworkManager::OnHostArenaPlayerSnapshot(SOCKET sock, const char* packetData, size_t packetSize)
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

    std::string playerName = NetworkManager::GetInstance().GetPlayerNameForSocket(sock);
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

void ArenaNetworkManager::OnHostArenaAttack(SOCKET sock, const Pkt_ArenaAttack& pkt)
{
    std::string attackerName = NetworkManager::GetInstance().GetPlayerNameForSocket(sock);
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
        NetworkManager::GetInstance().SendStateChangeToPlayer(targetName, EGameState::ArenaWait);
    }

    IPCManager::GetInstance().SendLog(
        "[아레나] 공격: " + attackerName + " -> " + targetName + " (-" + std::to_string(damage) + ")");

    TryEndArenaBattleIfNeeded();
    if (!arenaBattleStarted) return;

    AdvanceArenaTurnAfterAction();
}

void ArenaNetworkManager::OnHostArenaItemUse(SOCKET sock, const Pkt_ArenaItemUse& pkt)
{
    std::string userName = NetworkManager::GetInstance().GetPlayerNameForSocket(sock);
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
        targetHdr->hp += effectValue * targetHdr->maxHp / 100;
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

#pragma endregion

#pragma region Battle Flow

void ArenaNetworkManager::TryEndArenaBattleIfNeeded()
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
    NetworkManager::GetInstance().ApplySyncedStateChange(EGameState::ArenaResult);
}

void ArenaNetworkManager::TryStartArenaBattleAfterSnapshots()
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

    NetworkManager::GetInstance().ApplySyncedStateChange(EGameState::ArenaBattle);

    BroadcastArenaPlayerList(listPkt);

    StartFirstArenaTurn();
}

void ArenaNetworkManager::StartFirstArenaTurn()
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

void ArenaNetworkManager::AdvanceArenaTurnAfterAction()
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

bool ArenaNetworkManager::RequestAllArenaPlayerSnapshots(Player* hostPlayer)
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

void ArenaNetworkManager::SendArenaSessionApplyToAllPlayers(const Pkt_ArenaRankList& rankPkt)
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
            NetworkManager::GetInstance().SendToPlayerByName(name, buffer.data(), buffer.size());
        }
    }

    ArenaBattleManager::GetInstance().ClearBettedItems();
}

#pragma endregion

#pragma region C2S Send

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

void ArenaNetworkManager::SendArenaItemRegisterPacket(const std::string& itemName, int count,
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
    else
    {
        NetworkManager::GetInstance().SendToServer(&pkt, pkt.header.size);
    }
}

void ArenaNetworkManager::SendArenaLobbyArrivedPacket()
{
    if (Client::isServer)
    {
        OnHostArenaLobbyArrived(INVALID_SOCKET);
    }
    else
    {
        Pkt_ArenaReady pkt;
        NetworkManager::GetInstance().SendToServer(&pkt, pkt.header.size);
    }
}

void ArenaNetworkManager::SendArenaPlayerSnapshotPacket(Player* player)
{
    std::vector<char> buffer = BuildArenaPlayerSnapshotPacket(player);
    if (buffer.empty()) return;

    if (Client::isServer)
    {
        OnHostArenaPlayerSnapshot(INVALID_SOCKET, buffer.data(), buffer.size());
    }
    else
    {
        NetworkManager::GetInstance().SendToServer(buffer.data(), buffer.size());
    }
}

void ArenaNetworkManager::SendArenaAttackPacket(const std::string& targetName)
{
    Pkt_ArenaAttack pkt;
    CopyStringToPacketField(pkt.targetName, sizeof(pkt.targetName), targetName);

    if (Client::isServer)
    {
        OnHostArenaAttack(INVALID_SOCKET, pkt);
    }
    else
    {
        NetworkManager::GetInstance().SendToServer(&pkt, pkt.header.size);
    }
}

void ArenaNetworkManager::SendArenaItemUsePacket(const std::string& itemName, const std::string& targetName)
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
        NetworkManager::GetInstance().SendToServer(&pkt, pkt.header.size);
    }
}

void ArenaNetworkManager::CancelArenaPreparation()
{
    if (!Client::isServer) return;

    ArenaBattleManager& arena = ArenaBattleManager::GetInstance();
    const auto betsByPlayer = arena.GetBetsByPlayer();

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
            NetworkManager::GetInstance().SendToPlayerByName(pair.first, &refundPkt, refundPkt.header.size);
        }
    }

    arena.ClearAllArenaBets();
    ResetArenaLobbyArrivalTracking();

    IPCManager::GetInstance().SendLog("\033[1;34m방장이 아레나를 취소했습니다. 베팅 아이템을 반환합니다.\033[0m");
    NetworkManager::GetInstance().ApplySyncedStateChange(EGameState::Start);
}

#pragma endregion
