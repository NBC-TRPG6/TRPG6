#pragma once
#include <string>
#include "Inventory.h"

class Player;

class Shop
{
public:
    //생성자
    Shop();

    // 소멸자
    virtual ~Shop() {}

    void ShowStock() const;
    bool BuyItem(Player* player, const std::string& itemName);
    bool SellItem(Player* player, const std::string& itemName, int amount = 1);

private:
    Inventory<Item> stock; // 상점 재고
};
