// 게임 시작시 기본적으로 전환되는 상태

#pragma once
#include <string>
#include "IGameState.h"

class GameStartState : public IGameState {
public:
    void Update(int ch, std::string& lastCommand) override;
    void Enter() override;
};
