#include "ClearState.h"
#include "Renderer.h"
#include "Utils.h"
#include "GameManager.h"

void ClearState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\Win.png");
    Renderer::SetTopASCIIImage(art);
}

void ClearState::Update(int ch, std::string& lastCommand)
{
    static auto deathTime = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - deathTime).count() >= 5)
    {
        GameManager::GetInstance().SetIsGameRunning(false); // 게임 종료
    }
}
