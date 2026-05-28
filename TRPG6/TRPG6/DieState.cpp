#include "DieState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "Player.h"
#include <chrono>
#include "Utils.h"

void DieState::Enter()
{
    auto art = LoadImageAsASCIIColor("..\\..\\Resources\\Die.png");
    Renderer::SetTopASCIIImage(art);
}

void DieState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    //Renderer::DisplayUI(UIPart::CenterLeft, 2, "\033[31m==========\033[0m");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "\033[31mYOU DIED...\033[0m");
    //Renderer::DisplayUI(UIPart::CenterLeft, 4, "\033[31m==========\033[0m");


    GameManager::GetInstance().GetPlayer()->PrintStatus();

    static auto deathTime = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - deathTime).count() >= 5)
    {
        GameManager::GetInstance().SetIsGameRunning(false); // 게임 종료
    }
}
