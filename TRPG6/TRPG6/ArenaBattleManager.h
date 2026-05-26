#pragma once
#include <vector>
#include <string>
#include "Packet.h"

class ArenaBattleManager {
public:
    static ArenaBattleManager& GetInstance();

    ArenaBattleManager(const ArenaBattleManager&) = delete;
    ArenaBattleManager& operator=(const ArenaBattleManager&) = delete;

    void ResetSession();

    void AddBettedItem(const std::string& itemName, int amount);
    const std::vector<ArenaItemSlot>& GetBettedItems() const { return bettedItems; }

private:
    ArenaBattleManager() = default;
    ~ArenaBattleManager() = default;

    std::vector<ArenaItemSlot> bettedItems;
};
