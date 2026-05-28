#pragma once
#include "IGameState.h"
#include <string>

class COOPRewardState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
};