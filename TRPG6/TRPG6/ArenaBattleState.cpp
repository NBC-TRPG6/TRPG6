#include "ArenaBattleState.h"
#include "ArenaWaitState.h"
#include "ArenaResultState.h"
#include "NetworkManager.h"
#include "GameManager.h"
#include "Renderer.h"
#include "Packet.h"
#include "DATABASE.h"

/// <summary>
/// 아레나 배틀스테이트 진입 시 작동하는 함수
/// </summary>
void ArenaBattleState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    //턴 대기
    CurrentStep = BattleUIStep::WaitingTurn;
    CurrentTurnName = "";
    LastAttackLog = "";

    // 스냅샷 빌드 + 전송 + 로컬 복사
    Player* player = GameManager::GetInstance().GetPlayer();
    NetworkManager::GetInstance().SendArenaPlayerSnapshotPacket(player);
}

void ArenaBattleState::Update(int ch, std::string& lastCommand)
{
    DrawPlayerList();

    //내 턴인지 체크하는 함수(CurrentTurnName == Client::playerName)
    bool isMyTurn = (CurrentTurnName == Client::playerName);

    switch (CurrentStep)
    {
    case BattleUIStep::WaitingTurn:  //턴 대기중일 시
    {
        if (isMyTurn) // 내 턴이면
        {
            CurrentStep = BattleUIStep::MainMenu;
        }
        else
        {
            Renderer::DisplayUI(UIPart::CenterLeft, 8,
                CurrentTurnName.empty()
                ? "대기 중..."
                : CurrentTurnName + "의 턴입니다.");
        }
        break;
    }

    case BattleUIStep::MainMenu: // 내 턴일 때
    {
        DrawMainMenu();
        if (ch == 1)      CurrentStep = BattleUIStep::SelectTarget; // 1번 타겟 선택
        else if (ch == 2) CurrentStep = BattleUIStep::SelectItem; // 2번 아이템 선택
        break;
    }

    // 타겟 선택 시
    case BattleUIStep::SelectTarget:
    {
        DrawTargetList();

        int idx = 0;
        for (const auto& p : PlayerList)
        {
            if (!p.isAlive) continue;
            if (std::string(p.playerName) == Client::playerName) continue;
            ++idx;
            if (ch == idx)
            {
                NetworkManager::GetInstance().SendArenaAttackPacket(p.playerName);
                CurrentStep = BattleUIStep::WaitingTurn;
                break;
            }
        }
        break;
    }

    //아이템 사용 선택 시
    case BattleUIStep::SelectItem:
    {
        DrawItemList();

        if (ch >= 1 && ch <= (int)ItemSnapshot.size())
        {
            auto& slot = ItemSnapshot[ch - 1];
            if (slot.count > 0)
            {
                NetworkManager::GetInstance().SendArenaItemUsePacket(slot.itemName, "");
                slot.count--;
                CurrentStep = BattleUIStep::WaitingTurn;
            }
            else
            {
                Renderer::DisplayUI(UIPart::CenterLeft, 15, "해당 아이템이 없습니다.");
            }
        }
        break;
    }
    }
}

void ArenaBattleState::Exit()
{
    PlayerList.clear();
    ItemSnapshot.clear();
    CurrentTurnName = "";
    LastAttackLog = "";
}

// S2C 콜백 ================================================

void ArenaBattleState::OnPlayerList(const std::vector<ArenaPlayerListEntry>& playerStats)
{
    PlayerList.assign(playerStats.begin(), playerStats.end());
}

void ArenaBattleState::OnTurnStart(const std::string& turnPlayerName)
{
    CurrentTurnName = turnPlayerName;
    CurrentStep = BattleUIStep::WaitingTurn;
    Renderer::ClearAllCenterLeftUI();
}

void ArenaBattleState::OnHPSync(const std::vector<ArenaPlayerListEntry>& playerStats)
{
    PlayerList.assign(playerStats.begin(), playerStats.end());
}

void ArenaBattleState::OnAttackResult(const std::string& attacker, const std::string& target, int damage)
{
    LastAttackLog = attacker + "이(가) " + target + "을(를) 공격! 데미지: " + std::to_string(damage);
}

void ArenaBattleState::OnItemList(const std::vector<ArenaItemSlot>& items)
{
    ItemSnapshot.assign(items.begin(), items.end());
}

void ArenaBattleState::OnPlayerDie(const std::string& playerName)
{
    for (auto& p : PlayerList)
    {
        if (std::string(p.playerName) == playerName)
        {
            p.isAlive = 0;
            return;
        }
    }
}

// 드로우 함수 ================================================

void ArenaBattleState::DrawPlayerList()
{
    Renderer::DisplayUI(UIPart::Top, 0, "=== 아레나 전투 ===");

    int row = 1;
    for (const auto& p : PlayerList)
    {
        std::string line = std::string(p.playerName)
            + "  HP: " + std::to_string(p.hp)
            + " / " + std::to_string(p.maxHp);
        if (!p.isAlive) line += "  [탈락]";
        Renderer::DisplayUI(UIPart::Top, row++, line);
    }

    if (!LastAttackLog.empty())
    {
        Renderer::DisplayUI(UIPart::Top, row + 1, "[" + LastAttackLog + "]");
    }
}

void ArenaBattleState::DrawMainMenu()
{
    Renderer::DisplayUI(UIPart::CenterLeft, 8, "--- 내 턴 ---");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 공격");
    Renderer::DisplayUI(UIPart::CenterLeft, 10, "2. 아이템 사용");
}

void ArenaBattleState::DrawTargetList()
{
    Renderer::DisplayUI(UIPart::CenterLeft, 8, "--- 공격 대상 선택 ---");
    int idx = 1;
    for (const auto& p : PlayerList)
    {
        if (!p.isAlive) continue;
        if (std::string(p.playerName) == Client::playerName) continue;
        Renderer::DisplayUI(UIPart::CenterLeft, 8 + idx,
            std::to_string(idx) + ". "
            + p.playerName
            + "  HP: " + std::to_string(p.hp));
        ++idx;
    }
}

void ArenaBattleState::DrawItemList()
{
    Renderer::DisplayUI(UIPart::CenterLeft, 8, "--- 아이템 선택 ---");
    for (int i = 0; i < (int)ItemSnapshot.size(); ++i)
    {
        const auto& slot = ItemSnapshot[i];
        if (slot.count <= 0) continue;
        Renderer::DisplayUI(UIPart::CenterLeft, 9 + i,
            std::to_string(i + 1) + ". "
            + slot.itemName
            + "  x" + std::to_string(slot.count));
    }
}
