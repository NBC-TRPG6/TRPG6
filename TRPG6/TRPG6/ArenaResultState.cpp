#include "ArenaResultState.h"
#include "ArenaBattleManager.h"
#include "GameStartState.h"
#include "GameManager.h"
#include "Renderer.h"

void ArenaResultState::Enter() {
    Renderer::ClearAllCenterLeftUI();
    GameManager::GetInstance().GetPlayer()->PrintStatus();
}

// ArenaBattleManager에 저장된 RankList(S2C) 순위 표시
void ArenaResultState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 결과");

    const auto& ranks = ArenaBattleManager::GetInstance().GetRankEntries();
    int line = 0;
    for (const ArenaRankEntry& entry : ranks)
    {
        if (line >= 8) break;
        Renderer::DisplayUI(UIPart::CenterLeft, line,
            std::to_string(entry.rank) + "위 " + std::string(entry.playerName));
        ++line;
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 메인화면으로");

    if (ch == 1) {
        GameManager::GetInstance().SetCurrentState(new GameStartState());
    }
}

void ArenaResultState::Exit() {
}
