#include "ArenaBettingState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "DATABASE.h"
#include "IPCManager.h"
#include "Player.h"
#include "Utils.h"
#include "ArenaReadyState.h"
#include "NetworkManager.h"

void ArenaBettingState::Enter()
{
    Renderer::ClearAllCenterLeftUI();

    auto art = LoadImageAsASCII("..\\..\\Resources\\Betting.png");
    Renderer::SetTopASCIIImage(art);
}

void ArenaBettingState::Update(int ch, std::string& lastCommand)
{
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 아이템 베팅");
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "[ 베팅할 아이템 선택 ]");

    Player* player = GameManager::GetInstance().GetPlayer();
    auto& inventory = player->GetInventory();
    const auto& slots = inventory.GetSlots();
    int size = (int)slots.size();

    for (int i = 0; i < size; ++i)
    {
        std::string name = slots[i].item->GetName();
        int count = slots[i].count;
        std::string info = std::to_string(i + 1) + ". " + name + " (x" + std::to_string(count) + ")";
        Renderer::DisplayUI(UIPart::CenterLeft, i + 3, info);
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 12, " 0. 뒤로가기");

    if (ch > 0 && ch <= size)
    {
        // 아이템 베팅 처리
        auto& selectedSlot = slots[ch - 1];
        std::string itemName = selectedSlot.item->GetName();
        
        // 서버로 패킷 전송
        NetworkManager::GetInstance().SendArenaItemRegisterPacket(itemName, 1);
        
        // 로컬 인벤토리에서 제거
        inventory.UseItem(nullptr, itemName, 1);

        Renderer::DisplayUITimed(UIPart::CenterLeft, 14, itemName + "을(를) 베팅했습니다!", 2.0f);

        // ArenaReadyState로 돌아가기 전에 hasBet 설정
        ArenaReadyState* readyState = new ArenaReadyState();
        readyState->SetHasBet(true);
        GameManager::GetInstance().SetCurrentState(readyState);
    }
    else if (ch == 0)
    {
        GameManager::GetInstance().SetCurrentState(new ArenaReadyState());
    }
}

void ArenaBettingState::Exit()
{
}
