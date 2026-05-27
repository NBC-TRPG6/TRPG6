#pragma once
#include "IGameState.h"

// 탈락한 플레이어 전용 상태. S2C로 갱신되는 관전 캐시를 읽기 전용 UI로 표시한다.
class ArenaWaitState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
    void Exit() override;

private:
    // CenterLeft/CenterRight에 HP·턴·전투 로그·관전 대상 표시
    void RenderSpectatorUI();
    // spectateIndex를 생존자 목록 범위 안으로 맞춤
    void ClampSpectateIndex();

    // GetAlivePlayerNames()[spectateIndex] 를 강조 표시
    int spectateIndex = 0;
};
