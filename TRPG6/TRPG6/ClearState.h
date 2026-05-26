#pragma once
#include "IGameState.h"


class ClearState : public IGameState
{
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;

};

