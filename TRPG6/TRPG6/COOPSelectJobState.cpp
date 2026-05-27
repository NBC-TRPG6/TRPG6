#include "COOPSelectJobState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "NetworkManager.h"
#include "COOPReadyState.h"
#include "COOPManager.h"
#include "Utils.h"

void COOPSelectJobState::Enter() {
    auto art = LoadImageAsASCII("..\\..\\Resources\\dragon.png");
    Renderer::SetTopASCIIImage(art);
}

void COOPSelectJobState::Update(int ch, std::string& lastCommand) {
    Renderer::ClearAllCenterLeftUI();
    Renderer::DisplayUI(UIPart::Top, 0, "=== [직업 선택 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "1. 기본");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "2. 탱커");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "3. 힐러");
    Renderer::DisplayUI(UIPart::CenterLeft, 6, "0. 돌아가기");

    if (ch == 1 || lastCommand == "1") {
        COOPManager::GetInstance().players[Client::playerName].job = PlayerJob::None;
    } else if (ch == 2 || lastCommand == "2") {
        COOPManager::GetInstance().players[Client::playerName].job = PlayerJob::Tanker;
    } else if (ch == 3 || lastCommand == "3") {
        COOPManager::GetInstance().players[Client::playerName].job = PlayerJob::Healer;
    }
    if ((ch >= 0 && ch <= 3) || (lastCommand >= "0" && lastCommand <= "3")) GameManager::GetInstance().SetCurrentState(new COOPReadyState());
}
