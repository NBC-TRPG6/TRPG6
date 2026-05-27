#pragma once
#include "IGameState.h"
#include <string>

class COOPBattleState : public IGameState {
private:
    enum class SubState { Main, ItemSelect, HealSelect };
    SubState currentSubState = SubState::Main;

public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
};