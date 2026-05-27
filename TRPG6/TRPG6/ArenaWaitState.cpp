#include "ArenaWaitState.h"
#include "ArenaBattleManager.h"
#include "ArenaResultState.h"
#include "GameManager.h"
#include "Renderer.h"
#include <string>

// 탈락 진입 시 UI 초기화, 첫 생존자를 관전 대상으로 설정
void ArenaWaitState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    spectateIndex = 0;
    ClampSpectateIndex();
}

void ArenaWaitState::ClampSpectateIndex()
{
    auto alive = ArenaBattleManager::GetInstance().GetAlivePlayerNames();
    if (alive.empty())
    {
        spectateIndex = 0;
        return;
    }
    if (spectateIndex < 0) spectateIndex = 0;
    if (spectateIndex >= static_cast<int>(alive.size()))
    {
        spectateIndex = static_cast<int>(alive.size()) - 1;
    }
}

void ArenaWaitState::RenderSpectatorUI()
{
    ArenaBattleManager& arena = ArenaBattleManager::GetInstance();
    auto alive = arena.GetAlivePlayerNames();
    int aliveCount = arena.GetAliveCount();

    std::string topLine = "[관전] 아레나 전투 관전";
    if (!arena.GetCurrentTurnPlayer().empty())
    {
        topLine += " | 턴: " + arena.GetCurrentTurnPlayer();
    }
    topLine += " | 생존 " + std::to_string(aliveCount) + "명";
    Renderer::DisplayUI(UIPart::Top, 0, topLine);

    int line = 0;
    for (const auto& pair : arena.GetSpectatorPlayers())
    {
        if (line >= 10) break;

        const std::string& name = pair.first;
        const ArenaPlayerListEntry& e = pair.second;
        std::string row = name + "  HP " + std::to_string(e.hp) + "/" + std::to_string(e.maxHp);
        if (e.isAlive == 0)
        {
            row += " (탈락)";
        }

        if (!alive.empty() && spectateIndex >= 0 && spectateIndex < static_cast<int>(alive.size())
            && name == alive[static_cast<size_t>(spectateIndex)])
        {
            row = "> " + row + " < 관전 중";
        }

        Renderer::DisplayUI(UIPart::CenterLeft, line, row);
        ++line;
    }

    if (line == 0)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 0, "전투 데이터 수신 대기 중...");
        ++line;
    }

    const auto& log = arena.GetCombatLog();
    int logLine = 0;
    for (int i = static_cast<int>(log.size()) - 1; i >= 0 && logLine < 6; --i, ++logLine)
    {
        Renderer::DisplayUI(UIPart::CenterRight, logLine, log[static_cast<size_t>(i)]);
    }

    if (!alive.empty())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 11, "1~: 생존자 관전 대상 변경 (숫자)");
    }
    Renderer::DisplayUI(UIPart::CenterLeft, 12, "전투 종료 시 자동으로 결과 화면으로 이동");
}

// RankList 수신(battleEnded) 시 Result로 전환. ch 1~N으로 관전 대상 변경
void ArenaWaitState::Update(int ch, std::string& lastCommand)
{
    (void)lastCommand;

    if (ArenaBattleManager::GetInstance().IsBattleEnded())
    {
        GameManager::GetInstance().SetCurrentState(new ArenaResultState());
        return;
    }

    auto alive = ArenaBattleManager::GetInstance().GetAlivePlayerNames();
    if (ch >= 1 && ch <= static_cast<int>(alive.size()))
    {
        spectateIndex = ch - 1;
        ClampSpectateIndex();
    }

    RenderSpectatorUI();
}

void ArenaWaitState::Exit()
{
}
