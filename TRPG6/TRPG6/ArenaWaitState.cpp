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
        if (line >= 8) break;

        const std::string& name = pair.first;
        const ArenaPlayerListEntry& e = pair.second;
        std::string row = name + "  HP " + std::to_string(e.hp) + "/" + std::to_string(e.maxHp);
        if (e.isAlive == 0)
        {
            row += " (탈락)";
        }

        Renderer::DisplayUI(UIPart::CenterLeft, line, row);
        ++line;
    }

    if (line == 0)
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 0, "전투 데이터 수신 대기 중...");
        ++line;
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 9, lastActionLog);

    Renderer::DisplayUI(UIPart::CenterLeft, 12, "전투 종료 시 자동으로 결과 화면으로 이동");
}

// RankList 수신(battleEnded) 시 Result로 전환
void ArenaWaitState::Update(int ch, std::string& lastCommand)
{
    (void)lastCommand;

    if (ArenaBattleManager::GetInstance().IsBattleEnded())
    {
        GameManager::GetInstance().SetCurrentState(new ArenaResultState());
        return;
    }


    RenderSpectatorUI();
}

void ArenaWaitState::OnAttackResult(const std::string& attacker, const std::string& target, int damage)
{
    lastActionLog = attacker + "이(가) " + target + "을(를) 공격! 데미지: " + std::to_string(damage);
}
void ArenaWaitState::OnItemResult(const std::string& userName, const std::string& itemName, int itemType, int value) {
    lastActionLog = userName + "이(가) " + itemName + " 사용! 효과: " +
        (itemType == 0 ? "HP 회복 " : "버프 ") + std::to_string(value);
}

void ArenaWaitState::Exit()
{
}
