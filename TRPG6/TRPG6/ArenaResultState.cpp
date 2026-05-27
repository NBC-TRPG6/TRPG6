#include "ArenaResultState.h"
#include "ArenaBattleManager.h"
#include "GameStartState.h"
#include "GameManager.h"
#include "Renderer.h"
#include "Player.h"

namespace {
    std::string FormatRewardLine(const ArenaItemSlot& slot)
    {
        return std::string(slot.itemName) + " x" + std::to_string(slot.count);
    }
}

void ArenaResultState::Enter() {
    Renderer::ClearAllCenterLeftUI();
    GameManager::GetInstance().GetPlayer()->PrintStatus();
}

// ArenaBattleManager에 저장된 RankList(S2C) 순위 표시
void ArenaResultState::Update(int ch, std::string& lastCommand) {

    ArenaBattleManager& arena = ArenaBattleManager::GetInstance();
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 결과");

    const auto& ranks = ArenaBattleManager::GetInstance().GetRankEntries();
    int line = 0;
    for (const ArenaRankEntry& entry : ranks)
    {
        if (line >= 8) break;
        Renderer::DisplayUI(UIPart::CenterLeft, line++,
            std::to_string(entry.rank) + "위 " + std::string(entry.playerName));
    }
    line++;
    Renderer::DisplayUI(UIPart::CenterLeft, line++, "[ 보상 목록 ]");

    const std::vector<ArenaItemSlot>& rewards = arena.GetRewardPoolDisplay();
    if(rewards.empty())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, line++, "보상 아이템이 없습니다.");
    }
    else
    {
        for (const ArenaItemSlot& slot : rewards)
        {
            if (line >= 11) break;
            Renderer::DisplayUI(UIPart::CenterLeft, line++, FormatRewardLine(slot));
        }
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 12, "1. 메인화면으로");

    if (ch == 1) {
        GameManager::GetInstance().SetCurrentState(new GameStartState());
    }
}

void ArenaResultState::Exit() {
}
