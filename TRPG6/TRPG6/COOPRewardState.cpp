#include "COOPRewardState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "NetworkManager.h"
#include "GameStartState.h"
#include "COOPManager.h"
#include "Player.h"
#include "Utils.h"

void COOPRewardState::Enter() {
    auto art = LoadImageAsASCII("..\\..\\Resources\\RAIDReward.jpg");
    Renderer::SetTopASCIIImage(art);
    Player* p = GameManager::GetInstance().GetPlayer();
    if (p) NetworkManager::GetInstance().SendCOOPUpdateStatus(Client::playerName, p->GetAttack(), p->GetHp(), p->GetMaxHp(), static_cast<int>(COOPManager::GetInstance().GetMyJob()), false);
    
    if (Client::isServer) {
        for (auto& pair : COOPManager::GetInstance().players) {
            NetworkManager::GetInstance().BroadcastCOOPTakeItem(pair.first, "전리품 아이템 넣는 곳");
        }
    }
}

void COOPRewardState::Update(int ch, std::string& lastCommand) {
    Renderer::ClearAllCenterLeftUI();
    Renderer::DisplayUI(UIPart::Top, 0, "=== [ 레이드 보상 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "보스를 물리쳤습니다! 레이드 보상을 획득합니다.");
    Renderer::DisplayUI(UIPart::CenterLeft, 5, "0. 메인 화면으로 돌아가기");

    if (ch == 0 || lastCommand == "0") GameManager::GetInstance().SetCurrentState(new GameStartState());
}
