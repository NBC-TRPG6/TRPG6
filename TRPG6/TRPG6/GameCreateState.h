#pragma once
#include "IGameState.h"
#include <string>

class GameCreateState : public IGameState {
public:
    // 기본값은 0(이름 입력), 뒤로가기 시 1(방 선택)로 생성 가능
    GameCreateState(int startPhase = 0) : createPhase(startPhase) {}

    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;

private:
    int createPhase = 0;
    void ShowTitle();
    void ShowMenu();
};
