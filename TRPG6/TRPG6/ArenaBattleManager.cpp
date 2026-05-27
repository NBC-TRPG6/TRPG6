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
    spectatorPlayers.clear();
    currentTurnPlayer.clear();
    combatLog.clear();
    rankEntries.clear();
    eliminationOrder.clear();
    battleEnded = false;
}

void ArenaBattleManager::AddBettedItem(const std::string& itemName, int amount)
{
    for (auto& item : bettedItems)
    {
        if (std::string(item.itemName) == itemName)
        {
            item.count += amount;
            return;
        }
    }

    ArenaItemSlot newItem;
    CopyStringToPacketField(newItem.itemName, sizeof(newItem.itemName), itemName);
    newItem.count = amount;
    newItem.itemType = 0;
    newItem.value = 0;

    bettedItems.push_back(newItem);
}

void ArenaBattleManager::ClearBettedItems()
{
    bettedItems.clear();
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
        return;
    }

    entry->hp = pkt.currentHp;
    entry->maxHp = pkt.maxHp;
    entry->isAlive = pkt.currentHp > 0 ? 1 : 0;
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

// 1위: 유일 생존자 또는 전원 사망 시 마지막 탈락자, 2위 이하: eliminationOrder 역순
bool ArenaBattleManager::TryBuildRankList(Pkt_ArenaRankList& out) const
{
    out = Pkt_ArenaRankList();
    if (spectatorPlayers.size() < 2) return false;

    uint8_t idx = 0;
    int32_t nextRank = 1;

    auto alive = GetAlivePlayerNames();
    if (alive.size() == 1)
    {
        if (idx < MAX_ARENA_PLAYERS)
        {
            CopyStringToPacketField(out.entries[idx].playerName,
                sizeof(out.entries[idx].playerName), alive[0]);
            out.entries[idx].rank = 1;
            ++idx;
        }
        nextRank = 2;
    }
    else if (alive.empty() && !eliminationOrder.empty())
    {
        const std::string& lastStanding = eliminationOrder.back();
        if (idx < MAX_ARENA_PLAYERS)
        {
            CopyStringToPacketField(out.entries[idx].playerName,
                sizeof(out.entries[idx].playerName), lastStanding);
            out.entries[idx].rank = 1;
            ++idx;
        }
        nextRank = 2;
    }

    for (int i = static_cast<int>(eliminationOrder.size()) - 1; i >= 0; --i)
    {
        const std::string& name = eliminationOrder[static_cast<size_t>(i)];
        if (alive.size() == 1 && name == alive[0]) continue;
        if (alive.empty() && !eliminationOrder.empty() && name == eliminationOrder.back()) continue;

        if (idx >= MAX_ARENA_PLAYERS) break;
        CopyStringToPacketField(out.entries[idx].playerName,
            sizeof(out.entries[idx].playerName), name);
        out.entries[idx].rank = nextRank++;
        ++idx;
    }

    out.entryCount = idx;
    return idx > 0;
}
