#include "ShopBranchState.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "ShopState.h"
#include "Renderer.h"
#include "Utils.h"

void ShopBranchState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\nemo.png");
    Renderer::SetTopASCIIImage(art);
}

void ShopBranchState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "전투 종료! 상점에 방문하시겠습니까?");
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "0. 메인으로 가기");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "1. 상점으로 이동하기");

    switch (ch)
    {
    case 0:
        GameManager::GetInstance().SetCurrentState(new GameStartState());
        break;
    case 1:
        GameManager::GetInstance().SetCurrentState(new ShopState());
        break;
    default:
        break;
    }
}
