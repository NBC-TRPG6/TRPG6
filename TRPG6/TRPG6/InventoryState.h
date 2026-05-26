#pragma once
#include "IGameState.h"

class InventoryState : public IGameState
{
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
};
