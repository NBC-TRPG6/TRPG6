#include "ArenaBattleManager.h"

ArenaBattleManager& ArenaBattleManager::GetInstance()
{
    static ArenaBattleManager instance;
    return instance;
}

void ArenaBattleManager::ResetSession()
{
    bettedItems.clear();
}

void ArenaBattleManager::AddBettedItem(const std::string& itemName, int amount)
{
    // 이미 있는 아이템이면 개수만 추가
    for (auto& item : bettedItems)
    {
        if (std::string(item.itemName) == itemName)
        {
            item.count += amount;
            return;
        }
    }

    // 없으면 새로 추가
    ArenaItemSlot newItem;
    CopyStringToPacketField(newItem.itemName, sizeof(newItem.itemName), itemName);
    newItem.count = amount;
    // itemType과 value는 나중에 실제 아이템 데이터에서 가져와야 할 수도 있지만, 
    // 여기서는 간단하게 이름과 개수만 저장하거나 기본값 설정
    newItem.itemType = 0; 
    newItem.value = 0;

    bettedItems.push_back(newItem);
}
