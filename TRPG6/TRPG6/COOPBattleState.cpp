#include "COOPBattleState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "NetworkManager.h"
#include "COOPManager.h"
#include "Player.h"
#include "Utils.h"

void COOPBattleState::Enter() {
    auto art = LoadImageAsASCII("..\\..\\Resources\\dragon.png");
    Renderer::SetTopASCIIImage(art);
    currentSubState = SubState::Main;
}

void COOPBattleState::Update(int ch, std::string& lastCommand) {
    Renderer::ClearAllCenterLeftUI();
    GameManager::GetInstance().GetPlayer()->PrintStatus();
    
    auto& coop = COOPManager::GetInstance();
    Renderer::DisplayUI(UIPart::Top, 0, "보스: " + coop.currentBossName + " | HP: " + std::to_string(coop.currentBossHp));
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "현재 턴: " + coop.currentTurnPlayer);

    int pRow = 2;
    for (const auto& pair : coop.players)
    {
        std::string pState = pair.second.isDead ? "[사망]" : ("HP: " + std::to_string(pair.second.hp));
        std::string pJob = "";
        if (pair.second.job == PlayerJob::Tanker) pJob = "(탱커)";
        else if (pair.second.job == PlayerJob::Healer) pJob = "(힐러)";

        Renderer::DisplayUI(UIPart::CenterLeft, pRow++, pair.first + " " + pJob + " : " + pState);
    }

    auto myInfo = coop.players[Client::playerName];
    if (myInfo.isDead) {
        Renderer::DisplayUI(UIPart::CenterLeft, 1, "[ 관전 모드 ] 당신은 쓰러졌습니다.");
        return;
    }
    
    if (coop.IsMyTurn()) {
        if (currentSubState == SubState::Main) {
            Renderer::DisplayUI(UIPart::CenterLeft, 0, "당신의 턴입니다!");
            Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 공격");
            Renderer::DisplayUI(UIPart::CenterLeft, 10, "2. 방어 (탱커 전용)");
            Renderer::DisplayUI(UIPart::CenterLeft, 11, "3. 힐 (힐러 전용)");
            Renderer::DisplayUI(UIPart::CenterLeft, 12, "4. 아이템 사용");

            if (ch == 1 || lastCommand == "1") {
                NetworkManager::GetInstance().SendCOOPUseAttack(Client::playerName, coop.currentBossName, myInfo.atk);
                lastCommand = "";
            } else if (ch == 2 || lastCommand == "2") {
                if (myInfo.job == PlayerJob::Tanker) NetworkManager::GetInstance().SendCOOPUseBlock(Client::playerName, "ANY");
                else Renderer::DisplayUITimed(UIPart::CenterLeft, 11, "탱커만 사용할 수 있습니다.", 2.0f);
                lastCommand = "";
            } else if (ch == 3 || lastCommand == "3") {
                if (myInfo.job == PlayerJob::Healer) {
                    currentSubState = SubState::HealSelect;
                } else {
                    Renderer::DisplayUITimed(UIPart::CenterLeft, 11, "힐러만 사용할 수 있습니다.", 2.0f);
                }
                lastCommand = "";
            } else if (ch == 4 || lastCommand == "4") {
                auto& inv = GameManager::GetInstance().GetPlayer()->GetInventory();
                if (inv.GetSlots().empty()) {
                    Renderer::DisplayUITimed(UIPart::CenterLeft, 11, "사용할 수 있는 아이템이 없습니다.", 2.0f);
                } else {
                    currentSubState = SubState::ItemSelect;
                }
                lastCommand = "";
            }
        } else if (currentSubState == SubState::ItemSelect) {
            Renderer::DisplayUI(UIPart::CenterLeft, 5, "--- 아이템 선택 ---");
            auto& slots = GameManager::GetInstance().GetPlayer()->GetInventory().GetSlots();
            for (int i = 0; i < (int)slots.size(); ++i) {
                std::string itemInfo = std::to_string(i + 1) + ". " + slots[i].item->GetName() + " (x" + std::to_string(slots[i].count) + ")";
                Renderer::DisplayUI(UIPart::CenterLeft, 6 + i, itemInfo);
            }
            Renderer::DisplayUI(UIPart::CenterLeft, 6 + (int)slots.size(), "0. 돌아가기");

            if (ch == 0 || lastCommand == "0") {
                currentSubState = SubState::Main;
                lastCommand = "";
            } else if (ch > 0 && ch <= (int)slots.size()) {
                int selectedIdx = ch - 1;
                auto* selectedItem = slots[selectedIdx].item;
                int itemValue = selectedItem->GetValue();
                std::string itemName = selectedItem->GetName();

                // 소모만 처리 (효과는 서버 브로드캐스트를 통해 동기화됨)
                GameManager::GetInstance().GetPlayer()->GetInventory().UseItem(nullptr, itemName, 1);

                NetworkManager::GetInstance().SendCOOPUseItem(Client::playerName, itemName, itemValue);
                currentSubState = SubState::Main;
                lastCommand = "";
            }
        } else if (currentSubState == SubState::HealSelect) {
            Renderer::DisplayUI(UIPart::CenterLeft, 5, "--- 치료할 대상 선택 ---");
            std::vector<std::string> alivePlayers;
            for (const auto& pair : coop.players) {
                if (!pair.second.isDead) alivePlayers.push_back(pair.first);
            }
            for (int i = 0; i < (int)alivePlayers.size(); ++i) {
                Renderer::DisplayUI(UIPart::CenterLeft, 6 + i, std::to_string(i + 1) + ". " + alivePlayers[i] + " (HP: " + std::to_string(coop.players[alivePlayers[i]].hp) + ")");
            }
            Renderer::DisplayUI(UIPart::CenterLeft, 6 + (int)alivePlayers.size(), "0. 돌아가기");

            if (ch == 0 || lastCommand == "0") {
                currentSubState = SubState::Main;
                lastCommand = "";
            } else if (ch > 0 && ch <= (int)alivePlayers.size()) {
                std::string targetName = alivePlayers[ch - 1];
                NetworkManager::GetInstance().SendCOOPUseHeal(Client::playerName, targetName, 50); // 기본 50 힐링
                currentSubState = SubState::Main;
                lastCommand = "";
            }
        }
    } else {
        currentSubState = SubState::Main;
        Renderer::DisplayUI(UIPart::CenterLeft, 0, "다른 플레이어의 턴을 대기 중입니다...");
    }
}
