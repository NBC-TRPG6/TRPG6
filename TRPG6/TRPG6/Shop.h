#pragma once
#include <string>
#include "Inventory.h"

class Player;

class Shop
{
public:
    // 싱글톤 인스턴스 가져오기
    static Shop& GetInstance()
    {
        static Shop instance;
        return instance;
    }

    virtual ~Shop() {}

    void ShowStock() const;
    bool BuyItem(Player* player, const std::string& itemName);
    bool SellItem(Player* player, const std::string& itemName, int amount = 1);

    // 인덱스 기반 함수
    int GetStockSize() const;
    std::string GetItemNameByIndex(int index) const;
    int GetItemPriceByIndex(int index) const;
    int GetItemStockByIndex(int index) const;

private:
    Shop();    // 싱글톤
    Inventory<Item> stock; // 상점 재고
};
