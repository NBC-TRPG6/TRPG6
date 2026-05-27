#pragma once
#include "IGameState.h"

// 탈락한 플레이어 전용 상태. S2C로 갱신되는 관전 캐시를 읽기 전용 UI로 표시한다.
class ArenaWaitState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
    void Exit() override;

    void OnAttackResult(const std::string& attacker, const std::string& target, int damage);
    void OnItemResult(const std::string& userName, const std::string& itemName, int itemType, int value);


private:
    // CenterLeft/CenterRight에 HP·턴·전투 로그·관전 대상 표시
    void RenderSpectatorUI();


    std::string lastActionLog;
};
