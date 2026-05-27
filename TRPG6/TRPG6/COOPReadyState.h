#pragma once
#include "IGameState.h"
#include <string>

class COOPReadyState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
};