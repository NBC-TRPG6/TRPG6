#pragma once
#include "IGameState.h"

class GameLobbyState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
};
