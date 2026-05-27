#include "InventoryState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "DATABASE.h"
#include "IPCManager.h"
#include "Player.h"
#include "Utils.h"
#include "GameStartState.h"
#include "WeaponItem.h"

void InventoryState::Enter()
{
    Renderer::ClearAllCenterLeftUI();

    auto art = LoadImageAsASCII("..\\..\\Resources\\Inventory.png");
    Renderer::SetTopASCIIImage(art);
}

void InventoryState::Update(int ch, std::string& lastCommand)
{
    Renderer::DisplayUI(UIPart::Top, 0, "인벤토리");
    Renderer::DisplayUI(UIPart::CenterLeft, 1, "[ 소지한 물품 ]");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "[ 무기를 선택하면 장착이 가능합니다.]");


    Player* player = GameManager::GetInstance().GetPlayer();
    auto& inventory = player->GetInventory();
    const auto& slots = inventory.GetSlots();
    int size = (int)slots.size();

    for (int i = 0; i < size; ++i)
    {
        std::string name = slots[i].item->GetName();
        int count = slots[i].count;
        int sellPrice = slots[i].item->GetSellPrice();
        std::string displayName;
        if (slots[i].item->GetType() == ItemType::WEAPON)
        {
            WeaponItem* w = dynamic_cast<WeaponItem*>(slots[i].item);
            if (w != nullptr)
                displayName = w->GetColoredName();
            else
                displayName = name;
        }
        else
        {
            displayName = name;
        }
        std::string info = std::to_string(i + 1) + ". " + displayName + " (x" + std::to_string(count) + ") | 가격: " + std::to_string(sellPrice) + "G";
        Renderer::DisplayUI(UIPart::CenterLeft, i + 3, info);
    }

    Renderer::DisplayUI(UIPart::CenterLeft, 12, " 0. 뒤로가기");

    if (ch == 0)
    {
        GameManager::GetInstance().SetCurrentState(new GameStartState());
    }
    if (ch > 0 && ch <= size)
    {
        auto* selectedItem = slots[ch - 1].item;
        if (selectedItem->GetType() == ItemType::WEAPON)
        {
            WeaponItem* weapon = dynamic_cast<WeaponItem*>(selectedItem);
            if (weapon != nullptr)
                player->EquipWeapon(weapon);
        }
    }
    {

        }

}
