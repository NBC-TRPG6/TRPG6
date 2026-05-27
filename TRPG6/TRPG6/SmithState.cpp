#include "SmithState.h"
#include "GameManager.h"
#include "Renderer.h"
#include "Player.h"
#include "WeaponTable.h"
#include "GameStartState.h"
#include "IPCManager.h"
#include "Utils.h"

void SmithState::Enter()
{

    auto art = LoadImageAsASCIIColor("..\\..\\Resources\\Smith.png");
    Renderer::SetTopASCIIImage(art);

    Renderer::ClearAllCenterLeftUI();
    CurrentStep = SmithUIStep::MainMenu;
    FirstSelectedIndex = -1;
    ResultMessage = "";
    filteredIndices.clear();
}

void SmithState::Update(int ch, std::string& lastCommand)
{
    Player* player = GameManager::GetInstance().GetPlayer();
    auto& inventory = player->GetInventory();
    const auto& slots = inventory.GetSlots();

    switch (CurrentStep)
    {
    case SmithUIStep::MainMenu:
    {
        DrawMainMenu();
        if (ch == 1)
        {
            // 조합 — MONSTER_PART 필터링 (개수 1개 이상)
            filteredIndices.clear();
            for (int i = 0; i < (int)slots.size(); ++i)
            {
                if (slots[i].item->GetType() == ItemType::MONSTER_PART)
                    filteredIndices.push_back(i);
            }
            if (filteredIndices.size() < 2)
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "재료가 부족합니다! (MONSTER_PART 2개 필요)", 2.0f);
                break;
            }
            CurrentStep = SmithUIStep::SelectCraftFirst;
            Renderer::ClearAllCenterLeftUI();
        }
        else if (ch == 2)
        {
            // 강화 — 무기 필터링 (개수 2개 이상인 슬롯 or 슬롯 2개 이상)
            filteredIndices.clear();
            for (int i = 0; i < (int)slots.size(); ++i)
            {
                if (slots[i].item->GetType() == ItemType::WEAPON)
                    filteredIndices.push_back(i);
            }
            // 무기 슬롯이 2개 이상이거나, 슬롯 1개인데 개수가 2개 이상인 경우
            int totalWeapons = 0;
            for (int idx : filteredIndices)
                totalWeapons += slots[idx].count;

            if (totalWeapons < 2)
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "무기가 부족합니다! (무기 2개 필요)", 2.0f);
                break;
            }
            CurrentStep = SmithUIStep::SelectUpgradeFirst;
            Renderer::ClearAllCenterLeftUI();
        }
        else if (ch == 0)
        {
            GameManager::GetInstance().SetCurrentState(new GameStartState());
        }
        break;
    }

    case SmithUIStep::SelectCraftFirst:
    {
        DrawItemList();
        Renderer::DisplayUI(UIPart::CenterLeft, 11, "0. 뒤로가기");
        if (ch == 0)
        {
            // 메인메뉴로 복귀
            CurrentStep = SmithUIStep::MainMenu;
            Renderer::ClearAllCenterLeftUI();
            break;
        }
        else if (ch >= 1 && ch <= (int)filteredIndices.size())
        {
            // 첫 번째 재료 선택
            FirstSelectedIndex = filteredIndices[ch - 1];
            CurrentStep = SmithUIStep::SelectCraftSecond;
            Renderer::ClearAllCenterLeftUI();
        }
        break;
    }

    case SmithUIStep::SelectCraftSecond:
    {
       // IPCManager::GetInstance().SendLog("[디버그] SelectCraftSecond 진입 ch: " + std::to_string(ch) + " FirstSelectedIndex: " + std::to_string(FirstSelectedIndex));
        DrawItemList();
        Renderer::DisplayUI(UIPart::CenterLeft, 11, "0. 뒤로가기");
        if (ch == 0)
        {
            // 첫 번째 선택으로 복귀
            CurrentStep = SmithUIStep::SelectCraftFirst;
            FirstSelectedIndex = -1;
            Renderer::ClearAllCenterLeftUI();
            break;
        }
        else if (ch >= 1 && ch <= (int)filteredIndices.size())
        {
            int secondIndex = filteredIndices[ch - 1];

            // 같은 슬롯 선택 시 개수가 2개 이상인지 확인
            if (secondIndex == FirstSelectedIndex && slots[secondIndex].count < 2)
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "같은 아이템은 2개 이상 있어야 합니다!", 2.0f);
                break;
            }

            std::string name1 = slots[FirstSelectedIndex].item->GetName();
            std::string name2 = slots[secondIndex].item->GetName();
            int price1 = slots[FirstSelectedIndex].item->GetPrice();
            int price2 = slots[secondIndex].item->GetPrice();

            //IPCManager::GetInstance().SendLog("[디버그] name1: " + name1 + " name2: " + name2);
            //IPCManager::GetInstance().SendLog("[디버그] price1: " + std::to_string(price1) + " price2: " + std::to_string(price2));

            // 조합 실행
            WeaponItem* result = WeaponTable::GetInstance().Craft(price1, price2);

            std::string resultName = result->GetName();

            // 인벤토리에서 재료 제거
            player->RemoveItem(name1, 1);
            player->RemoveItem(name2, 1);

            // 결과 무기 인벤토리 추가
            inventory.AddItem(result, 1);

            //조합 성공 띄우고 초기화하기
            ResultMessage = "조합 성공! [" + resultName + "] 획득!";
            CurrentStep = SmithUIStep::ShowResult;
            filteredIndices.clear();
            FirstSelectedIndex = -1;
            Renderer::ClearAllCenterLeftUI();
        }
        break;
    }

    case SmithUIStep::SelectUpgradeFirst:
    {
        DrawItemList();
        Renderer::DisplayUI(UIPart::CenterLeft, 11, "0. 뒤로가기");
        if (ch == 0)
        {
            // 메인메뉴로 복귀
            CurrentStep = SmithUIStep::MainMenu;
            Renderer::ClearAllCenterLeftUI();
            break;
        }
        else if (ch >= 1 && ch <= (int)filteredIndices.size())
        {
            // 첫 번째 무기 선택
            FirstSelectedIndex = filteredIndices[ch - 1];
            CurrentStep = SmithUIStep::SelectUpgradeSecond;
            Renderer::ClearAllCenterLeftUI();
        }
        break;
    }

    case SmithUIStep::SelectUpgradeSecond:
    {
        DrawItemList();
        Renderer::DisplayUI(UIPart::CenterLeft, 11, "0. 뒤로가기");
        if (ch == 0)
        {
            // 첫 번째 선택으로 복귀
            CurrentStep = SmithUIStep::SelectUpgradeFirst;
            FirstSelectedIndex = -1;
            Renderer::ClearAllCenterLeftUI();
            break;
        }
        if (ch >= 1 && ch <= (int)filteredIndices.size())
        {
            int secondIndex = filteredIndices[ch - 1];

            // 같은 슬롯 선택 시 개수가 2개 이상인지 확인
            if (secondIndex == FirstSelectedIndex && slots[secondIndex].count < 2)
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "같은 무기는 2개 이상 있어야 합니다!", 2.0f);
                break;
            }

            WeaponItem* w1 = dynamic_cast<WeaponItem*>(slots[FirstSelectedIndex].item);
            WeaponItem* w2 = dynamic_cast<WeaponItem*>(slots[secondIndex].item);

            if (w1 == nullptr || w2 == nullptr)
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "무기 아이템이 아닙니다!", 2.0f);
                break;
            }

            // 1번 무기 기준으로 강화
            std::string name1 = w1->GetName();
            std::string name2 = w2->GetName();

            //디버깅용 로그
            //IPCManager::GetInstance().SendLog("[디버그] w1: " + w1->GetName());
            //IPCManager::GetInstance().SendLog("[디버그] w2: " + w2->GetName());

            WeaponItem* result = WeaponTable::GetInstance().Upgrade(w1, w2);
            if (result == nullptr)
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 12, "강화 불가! (이미 최대 강화 상태)", 2.0f);
                break;
            }

            std::string resultName = result->GetName();


            // 인벤토리에서 무기 2개 제거
            player->RemoveItem(name1, 1);
            player->RemoveItem(name2, 1);

            // 강화된 무기 추가
            inventory.AddItem(result, 1);

            ResultMessage = "강화 성공! [" + resultName + "] 획득!";
            CurrentStep = SmithUIStep::ShowResult;
            filteredIndices.clear();
            FirstSelectedIndex = -1;
            Renderer::ClearAllCenterLeftUI();
        }
        break;
    }

    case SmithUIStep::ShowResult:
    {
        Renderer::DisplayUI(UIPart::CenterLeft, 8, ResultMessage);
        Renderer::DisplayUI(UIPart::CenterLeft, 10, "0. 돌아가기");
        if (ch == 0)
        {
            // 메인메뉴로 복귀
            CurrentStep = SmithUIStep::MainMenu;
            Renderer::ClearAllCenterLeftUI();
        }
        break;
    }
    }
}

void SmithState::Exit()
{
    filteredIndices.clear();
    FirstSelectedIndex = -1;
    ResultMessage = "";
}

void SmithState::DrawMainMenu()
{
    Renderer::DisplayUI(UIPart::Top, 0, "=== 대장간 ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 8, "1. 조합 (MONSTER_PART 2개 → 랜덤 무기)");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "2. 강화 (무기 2개 → 강화)");
    Renderer::DisplayUI(UIPart::CenterLeft, 10, "0. 돌아가기");
}

void SmithState::DrawItemList()
{
    Player* player = GameManager::GetInstance().GetPlayer();
    const auto& slots = player->GetInventory().GetSlots();

    //IPCManager::GetInstance().SendLog("[디버그] DrawItemList slots.size(): " + std::to_string(slots.size()) + " filteredIndices.size(): " + std::to_string(filteredIndices.size()));

    Renderer::DisplayUI(UIPart::CenterLeft, 1, "[ 아이템 선택 ]");
    for (int i = 0; i < (int)filteredIndices.size(); ++i)
    {
        // filteredIndices[i]가 slots 범위를 벗어나는지 확인
        if (filteredIndices[i] >= (int)slots.size())
        {
           // IPCManager::GetInstance().SendLog("[디버그] 범위 초과! filteredIndices[" + std::to_string(i) + "]: " + std::to_string(filteredIndices[i]));
            continue;
        }
        const auto& slot = slots[filteredIndices[i]];
        std::string line = std::to_string(i + 1) + ". ";

        if (slot.item->GetType() == ItemType::WEAPON)
        {
            WeaponItem* w = dynamic_cast<WeaponItem*>(slot.item);
            if (w != nullptr)
                line += w->GetColoredName();
            else
                line += slot.item->GetName(); // 캐스팅 실패 시 일반 이름 출력
        }
        else
        {
            line += slot.item->GetName();
        }
        Renderer::DisplayUI(UIPart::CenterLeft, i + 3, line);
    }
}
