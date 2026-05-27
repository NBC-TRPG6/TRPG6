#pragma once
#include "IGameState.h"
#include "Packet.h"
#include <vector>
#include <string>

enum class BattleUIStep {
    WaitingTurn, // 내 턴 대기 중
    MainMenu,   // 공격,아이템 선택
    SelectTarget, //공격 타겟 선택
    SelectItem // 아이템 선택
};

class ArenaBattleState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
    void Exit() override;

    // NetworkManager가 S2C 수신 시 호출되는 함수들
    void OnPlayerList(const std::vector<ArenaPlayerListEntry>& playerStats);
    void OnTurnStart(const std::string& turnPlayerName);
    void OnHpSync(const std::string& playerName, int32_t currentHp, int32_t maxHp);
    void OnAttackResult(const std::string& attacker, const std::string& target, int damage);
    void OnItemList(const std::vector<ArenaItemSlot>& items);
    void OnItemResult(const std::string& userName, const std::string& itemName, int itemType, int value);
    void OnPlayerDie(const std::string& playerName);
    void DrawMyStatus();

private:
    void DrawPlayerList();
    void DrawMainMenu();
    void DrawTargetList();
    void DrawItemList();
    bool HasUsableArenaItems() const;

    /// <summary>
    /// 플레이어 기본: 대기(턴X)
    /// </summary>
    BattleUIStep CurrentStep = BattleUIStep::WaitingTurn;
    std::string CurrentTurnName;

    // S2C_PlayerList / S2C_HpSync 수신 시 갱신
    std::vector<ArenaPlayerListEntry> PlayerList;

    // Enter() 시 스냅샷에서 복사, OnItemList 수신 시 갱신
    std::vector<ArenaItemSlot> ItemSnapshot;

    // 마지막으로 공격한 로그(플레이어1이 플레이어2를 공격! 대미지:~~ )가 적히는 문자열
    std::string LastAttackLog;

    // 
    //서버가 턴 관리, 플레이어가 턴 사용하면 서버가 다음 턴 넘기기

};
