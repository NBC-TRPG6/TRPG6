#include "ArenaBattleState.h"
#include "NetworkManager.h"
#include "GameManager.h"
#include "Renderer.h"
#include "DATABASE.h"

void ArenaBattleState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
    CurrentStep = BattleUIStep::WaitingTurn;
    CurrentTurnName = "";
    LastAttackLog = "";
    PlayerList.clear();
    ItemSnapshot.clear();
}

void ArenaBattleState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    (void)lastCommand;
    DrawMyStatus();

    DrawPlayerList();

    bool isMyTurn = (CurrentTurnName == Client::playerName);

    switch (CurrentStep)
    {
    case BattleUIStep::WaitingTurn:
    {
        if (isMyTurn)
        {
            CurrentStep = BattleUIStep::MainMenu;
        }
        else
        {
            Renderer::DisplayUI(UIPart::CenterLeft, 8,
                CurrentTurnName.empty() ? "전투 데이터 수신 대기 중..." : CurrentTurnName + "의 턴입니다.");
        }
        break;
    }

    case BattleUIStep::MainMenu:
    {
        DrawMainMenu();
        if (ch == 1)
        {
            CurrentStep = BattleUIStep::SelectTarget;
        }
        else if (ch == 2)
        {
            if (HasUsableArenaItems())
            {
                CurrentStep = BattleUIStep::SelectItem;
            }
            else
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 11,
                    "사용 가능한 아이템이 없습니다.", 2.0f);
            }
        }
        break;
    }

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

    case BattleUIStep::SelectItem:
    {
        DrawItemList();

        int itemMenuIdx = 0;
        for (size_t i = 0; i < ItemSnapshot.size(); ++i)
        {
            if (ItemSnapshot[i].count <= 0) continue;
            ++itemMenuIdx;
            if (ch != itemMenuIdx) continue;

            auto& slot = ItemSnapshot[i];
            if (slot.count > 0)
            {
                NetworkManager::GetInstance().SendArenaItemUsePacket(slot.itemName, Client::playerName);
                CurrentStep = BattleUIStep::WaitingTurn;
            }
            else
            {
                Renderer::DisplayUI(UIPart::CenterLeft, 15, "해당 아이템이 없습니다.");
            }
            break;
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
    CurrentStep = BattleUIStep::WaitingTurn;
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
}

void ArenaBattleState::OnHpSync(const std::string& playerName, int32_t currentHp, int32_t maxHp)
{
    for (auto& p : PlayerList)
    {
        if (std::string(p.playerName) == playerName)
        {
            p.hp = currentHp;
            p.maxHp = maxHp;
            p.isAlive = currentHp > 0 ? 1 : 0;
            return;
        }
    }
}

void ArenaBattleState::OnAttackResult(const std::string& attacker, const std::string& target, int damage)
{
    LastAttackLog = attacker + "이(가) " + target + "을(를) 공격! 데미지: " + std::to_string(damage);
}

void ArenaBattleState::OnItemList(const std::vector<ArenaItemSlot>& items)
{
    ItemSnapshot.assign(items.begin(), items.end());
}

void ArenaBattleState::OnItemResult(const std::string& userName, const std::string& itemName, int itemType, int value)
{
    LastAttackLog = userName + "이(가) " + itemName + " 사용! 효과: " +
        (itemType == 0 ? "HP 회복 " : "버프 ") + std::to_string(value);
}

void ArenaBattleState::OnPlayerDie(const std::string& playerName)
{
    for (auto& p : PlayerList)
    {
        if (std::string(p.playerName) == playerName)
        {
            p.isAlive = 0;
            p.hp = 0;
            return;
        }
    }
}

bool ArenaBattleState::HasUsableArenaItems() const
{
    for (const auto& slot : ItemSnapshot)
    {
        if (slot.count > 0) return true;
    }
    return false;
}

void ArenaBattleState::DrawPlayerList()
{
    Renderer::DisplayUI(UIPart::Top, 0, "=== 아레나 전투 ===");

    if (PlayerList.empty())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 1, "플레이어 목록 대기 중...");
        return;
    }

    int row = 1;
    for (const auto& p : PlayerList)
    {
        std::string line = std::string(p.playerName)
            + "  HP: " + std::to_string(p.hp)
            + " / " + std::to_string(p.maxHp);
        if (!p.isAlive) line += "  [탈락]";
        Renderer::DisplayUI(UIPart::CenterLeft, row++, line);
    }

    if (!LastAttackLog.empty())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, row + 1, "[" + LastAttackLog + "]");
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
    int displayIdx = 1;
    for (size_t i = 0; i < ItemSnapshot.size(); ++i)
    {
        const auto& slot = ItemSnapshot[i];
        if (slot.count <= 0) continue;
        Renderer::DisplayUI(UIPart::CenterLeft, 8 + displayIdx,
            std::to_string(displayIdx) + ". "
            + slot.itemName
            + "  x" + std::to_string(slot.count));
        ++displayIdx;
    }
}

void ArenaBattleState::DrawMyStatus()
{
    for (const auto& p : PlayerList)
    {
        if (std::string(p.playerName) == Client::playerName)
        {
            Renderer::DisplayUI(UIPart::CenterRight, 0, "[ 내 정보 ]");
            Renderer::DisplayUI(UIPart::CenterRight, 1, "이름: " + std::string(p.playerName));
            Renderer::DisplayUI(UIPart::CenterRight, 2, "HP: " + std::to_string(p.hp) + "/" + std::to_string(p.maxHp));
            Renderer::DisplayUI(UIPart::CenterRight, 3, "공격력: " + std::to_string(p.attack));
            Renderer::DisplayUI(UIPart::CenterRight, 4, "레벨: " + std::to_string(p.level));
            return;
        }
    }
}
