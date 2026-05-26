#pragma once
#include "IGameState.h"

class ShopBranchState : public IGameState
{
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
};
