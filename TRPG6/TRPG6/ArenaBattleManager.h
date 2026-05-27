#pragma once
#include <vector>
#include <string>
#include <map>
#include "Packet.h"

// 아레나 베팅 아이템, 관전 캐시, 순위 등 세션 단위 데이터 관리
class ArenaBattleManager {
public:
    static ArenaBattleManager& GetInstance();

    ArenaBattleManager(const ArenaBattleManager&) = delete;
    ArenaBattleManager& operator=(const ArenaBattleManager&) = delete;

    // 베팅, 관전, 순위 등 아레나 세션 전체 초기화
    void ResetSession();

    // 보상 풀 + 플레이어별 베팅 기록(취소 시 반환용)
    void RegisterPlayerBet(const std::string& playerName, const ArenaItemSlot& slot);
    void ClearBettedItems();
    void ClearAllArenaBets();
    const std::vector<ArenaItemSlot>& GetBettedItems() const { return bettedItems; }
    const std::map<std::string, std::vector<ArenaItemSlot>>& GetBetsByPlayer() const { return betsByPlayer; }

    // S2C PlayerList: 관전용 플레이어 목록 갱신(전투 시작 시 1회)
    void OnSpectatorPlayerList(const Pkt_ArenaPlayerList& pkt);
    // S2C TurnStart: 현재 턴 플레이어 이름 저장
    void OnSpectatorTurnStart(const std::string& turnPlayerName);
    // S2C AttackResult: 전투 로그에 공격 결과 한 줄 추가
    void OnSpectatorAttackResult(const Pkt_ArenaAttackResult& pkt);
    // S2C HpSync: 해당 플레이어 HP, 생존 여부 갱신
    void OnSpectatorHpSync(const Pkt_ArenaHpSync& pkt);
    // S2C Die: 탈락 처리 및 eliminationOrder에 탈락 순서 기록
    void OnSpectatorDie(const std::string& playerName);
    // S2C RankList: 최종 순위 저장, battleEnded = true
    void OnSpectatorRankList(const Pkt_ArenaRankList& pkt);
    // S2C RewardPool: 보상 풀 아이템 목록 저장(세션 결과 표시용)
    void OnSpectatorRewardPool(const Pkt_ArenaRewardPool& pkt);

    const std::map<std::string, ArenaPlayerListEntry>& GetSpectatorPlayers() const { return spectatorPlayers; }
    const std::string& GetCurrentTurnPlayer() const { return currentTurnPlayer; }
    const std::vector<std::string>& GetCombatLog() const { return combatLog; }
    const std::vector<ArenaRankEntry>& GetRankEntries() const { return rankEntries; }
    const std::vector<ArenaItemSlot>& GetRewardPoolDisplay() const { return rewardPoolDisplay; }

    // isAlive != 0 인 플레이어 이름 목록(이름 오름차순)
    std::vector<std::string> GetAlivePlayerNames() const;
    // 현재 생존 인원 수
    int GetAliveCount() const;
    // RankList 수신 후 true, ArenaWait에서 Result 전환에 사용
    bool IsBattleEnded() const { return battleEnded; }
    // spectatorPlayers 전원 순위. 1위=유일 생존자 또는 최후 탈락자, 2위~=탈락 늦은 순. 2인 미만 false
    bool TryBuildRankList(Pkt_ArenaRankList& out) const;

private:
    ArenaBattleManager() = default;
    ~ArenaBattleManager() = default;

    // combatLog 최대 MaxCombatLogLines 줄 유지
    void PushCombatLog(const std::string& line);
    // spectatorPlayers에서 이름 검색, 없으면 nullptr
    ArenaPlayerListEntry* FindSpectatorPlayer(const std::string& playerName);

    // 아레나 보상(베팅) 풀에 올라간 아이템 목록
    std::vector<ArenaItemSlot> bettedItems;
    // 플레이어별 베팅 내역(준비 취소 시 인벤 반환)
    std::map<std::string, std::vector<ArenaItemSlot>> betsByPlayer;
    // 관전 UI용 플레이어별 HP, 공격력, 생존 여부
    std::map<std::string, ArenaPlayerListEntry> spectatorPlayers;
    // 현재 턴을 진행 중인 플레이어 이름
    std::string currentTurnPlayer;
    // 최근 전투 로그(공격, 탈락 등)
    std::vector<std::string> combatLog;
    // S2C RankList로 받은 최종 순위(ArenaResultState 표시용)
    std::vector<ArenaRankEntry> rankEntries;
    // 탈락 순서(먼저 죽은 사람이 앞). TryBuildRankList에서 역순으로 2위 이하 배정
    std::vector<std::string> eliminationOrder;
    // S2C RewardPool으로 받은 보상 풀 아이템 목록(ArenaResultState 표시용)
    std::vector<ArenaItemSlot> rewardPoolDisplay;
    // 전투 종료 여부(RankList 수신 시 true)
    bool battleEnded = false;

    // combatLog에 보관할 최대 줄 수
    static constexpr size_t MaxCombatLogLines = 8;
};
