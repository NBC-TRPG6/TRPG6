#include "ArenaBattleManager.h"
#include <algorithm>

ArenaBattleManager& ArenaBattleManager::GetInstance()
{
    static ArenaBattleManager instance;
    return instance;
}

// bettedItems, spectatorPlayers, eliminationOrder 등 전부 비움
void ArenaBattleManager::ResetSession()
{
    bettedItems.clear();
    betsByPlayer.clear();
    spectatorPlayers.clear();
    currentTurnPlayer.clear();
    combatLog.clear();
    rankEntries.clear();
    eliminationOrder.clear();
    rewardPoolDisplay.clear();
    battleEnded = false;
}

void ArenaBattleManager::RegisterPlayerBet(const std::string& playerName, const ArenaItemSlot& slot)
{
    if (slot.count <= 0) return;

    betsByPlayer[playerName].push_back(slot);

    for (auto& item : bettedItems)
    {
        if (std::string(item.itemName) == std::string(slot.itemName))
        {
            item.count += slot.count;
            return;
        }
    }
    bettedItems.push_back(slot);
}

void ArenaBattleManager::ClearBettedItems()
{
    bettedItems.clear();
}

void ArenaBattleManager::ClearAllArenaBets()
{
    bettedItems.clear();
    betsByPlayer.clear();
}

// 관전 UI용 전투 로그, 오래된 줄부터 삭제
void ArenaBattleManager::PushCombatLog(const std::string& line)
{
    combatLog.push_back(line);
    if (combatLog.size() > MaxCombatLogLines)
    {
        combatLog.erase(combatLog.begin());
    }
}

ArenaPlayerListEntry* ArenaBattleManager::FindSpectatorPlayer(const std::string& playerName)
{
    auto it = spectatorPlayers.find(playerName);
    if (it == spectatorPlayers.end()) return nullptr;
    return &it->second;
}

// 전투 시작 시 PlayerList로 관전 캐시 초기화
void ArenaBattleManager::OnSpectatorPlayerList(const Pkt_ArenaPlayerList& pkt)
{
    spectatorPlayers.clear();
    eliminationOrder.clear();
    battleEnded = false;
    for (uint8_t i = 0; i < pkt.playerCount && i < MAX_ARENA_PLAYERS; ++i)
    {
        std::string name = pkt.players[i].playerName;
        spectatorPlayers[name] = pkt.players[i];
    }
}

void ArenaBattleManager::OnSpectatorTurnStart(const std::string& turnPlayerName)
{
    currentTurnPlayer = turnPlayerName;
}

// "공격자 -> 피격자 (-데미지)" 형식으로 combatLog 추가
void ArenaBattleManager::OnSpectatorAttackResult(const Pkt_ArenaAttackResult& pkt)
{
    std::string line = std::string(pkt.attackerName) + " -> " + pkt.targetName +
        " (-" + std::to_string(pkt.damage) + ")";
    PushCombatLog(line);
}

// HP 동기화, 미등록 플레이어면 spectatorPlayers에 새 항목 생성
void ArenaBattleManager::OnSpectatorHpSync(const Pkt_ArenaHpSync& pkt)
{
    std::string name = pkt.playerName;
    ArenaPlayerListEntry* entry = FindSpectatorPlayer(name);
    if (entry == nullptr)
    {
        ArenaPlayerListEntry newEntry{};
        CopyStringToPacketField(newEntry.playerName, sizeof(newEntry.playerName), name);
        newEntry.hp = pkt.currentHp;
        newEntry.maxHp = pkt.maxHp;
        newEntry.isAlive = pkt.currentHp > 0 ? 1 : 0;
        spectatorPlayers[name] = newEntry;

        if (pkt.currentHp <= 0)
        {
            eliminationOrder.push_back(name);
        }
        return;
    }

    entry->hp = pkt.currentHp;
    entry->maxHp = pkt.maxHp;
    entry->isAlive = pkt.currentHp > 0 ? 1 : 0;

    if (pkt.currentHp <= 0 &&
        std::find(eliminationOrder.begin(), eliminationOrder.end(), name) == eliminationOrder.end())
    {
        eliminationOrder.push_back(name);
    }
}

// 탈락 표시 및 eliminationOrder에 순서 기록(중복 방지)
void ArenaBattleManager::OnSpectatorDie(const std::string& playerName)
{
    ArenaPlayerListEntry* entry = FindSpectatorPlayer(playerName);
    if (entry != nullptr)
    {
        entry->isAlive = 0;
        entry->hp = 0;
    }
    if (std::find(eliminationOrder.begin(), eliminationOrder.end(), playerName) == eliminationOrder.end())
    {
        eliminationOrder.push_back(playerName);
    }
    PushCombatLog(playerName + " 탈락");
}

// 최종 순위 저장 후 battleEnded = true (ArenaWait -> Result 전환 트리거)
void ArenaBattleManager::OnSpectatorRankList(const Pkt_ArenaRankList& pkt)
{
    rankEntries.clear();
    for (uint8_t i = 0; i < pkt.entryCount && i < MAX_ARENA_PLAYERS; ++i)
    {
        rankEntries.push_back(pkt.entries[i]);
    }
    battleEnded = true;
}

void ArenaBattleManager::OnSpectatorRewardPool(const Pkt_ArenaRewardPool& pkt)
{
    rewardPoolDisplay.clear();
    for (uint8_t i = 0; i < pkt.slotCount && i < MAX_ARENA_ITEM_SLOTS; ++i)
    {
        if (pkt.slots[i].count <= 0) continue;
        rewardPoolDisplay.push_back(pkt.slots[i]);
    }
}

// ArenaWaitState에서 관전 대상 선택용
std::vector<std::string> ArenaBattleManager::GetAlivePlayerNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : spectatorPlayers)
    {
        if (pair.second.isAlive != 0)
        {
            names.push_back(pair.first);
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

int ArenaBattleManager::GetAliveCount() const
{
    int count = 0;
    for (const auto& pair : spectatorPlayers)
    {
        if (pair.second.isAlive != 0) ++count;
    }
    return count;
}

// spectatorPlayers 전원 순위. 1위=유일 생존자 또는 전원 사망 시 마지막 탈락자, 나머지=탈락 늦은 순
bool ArenaBattleManager::TryBuildRankList(Pkt_ArenaRankList& out) const
{
    out = Pkt_ArenaRankList();

    const auto alive = GetAlivePlayerNames();
    std::string firstPlace;

    if (alive.size() == 1)
    {
        firstPlace = alive[0];
    }
    else if (alive.empty() && !eliminationOrder.empty())
    {
        firstPlace = eliminationOrder.back();
    }
    else
    {
        return false;
    }

    std::vector<std::string> rest;
    rest.reserve(spectatorPlayers.size());
    for (const auto& pair : spectatorPlayers)
    {
        if (pair.first != firstPlace)
        {
            rest.push_back(pair.first);
        }
    }

    const auto elimIndex = [this](const std::string& name) -> int
    {
        const auto it = std::find(eliminationOrder.begin(), eliminationOrder.end(), name);
        if (it == eliminationOrder.end()) return -1;
        return static_cast<int>(std::distance(eliminationOrder.begin(), it));
    };

    std::sort(rest.begin(), rest.end(),
        [&elimIndex](const std::string& a, const std::string& b)
        {
            const int ia = elimIndex(a);
            const int ib = elimIndex(b);
            if (ia < 0 && ib < 0) return a < b;
            if (ia < 0) return false;
            if (ib < 0) return true;
            return ia > ib;
        });

    uint8_t idx = 0;
    int32_t nextRank = 1;

    if (idx < MAX_ARENA_PLAYERS)
    {
        CopyStringToPacketField(out.entries[idx].playerName,
            sizeof(out.entries[idx].playerName), firstPlace);
        out.entries[idx].rank = nextRank++;
        ++idx;
    }

    for (const std::string& name : rest)
    {
        if (idx >= MAX_ARENA_PLAYERS) break;

        CopyStringToPacketField(out.entries[idx].playerName,
            sizeof(out.entries[idx].playerName), name);
        out.entries[idx].rank = nextRank++;
        ++idx;
    }

    out.entryCount = idx;
    return idx >= 2 && static_cast<size_t>(idx) == std::min(
        spectatorPlayers.size(), static_cast<size_t>(MAX_ARENA_PLAYERS));
}
