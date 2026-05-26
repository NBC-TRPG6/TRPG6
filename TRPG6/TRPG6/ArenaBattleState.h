#pragma once
#include "IGameState.h"

class ArenaBattleState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
    void Exit() override;
};
