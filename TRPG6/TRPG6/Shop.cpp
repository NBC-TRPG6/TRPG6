#include "Shop.h"
#include "Player.h"
#include <iostream>
#include <algorithm>

Shop::Shop()
{
    // 상점 초기 재고 설정(추후에 다른 곳에 옮겨 작성할지도..!!!)
    stock.AddItem(new Item("HP 포션", ItemType::HP_POTION, 50, 15), 30);
    stock.AddItem(new Item("공격력 포션", ItemType::ATTACK_BUFF, 10, 30), 30);
}

/*
 * @brief 상점 재고 출력 메서드
 */
void Shop::ShowStock() const
{
    std::cout << "[상점 재고 목록]" << std::endl;
    stock.PrintAllItems([](const Item* item, int count)
        {
            std::cout << "- " << item->GetName()
                << " | 가격: " << item->GetPrice() << "G"
                << " | 재고: " << count << "개" << std::endl;
        });
}

/*
 * @brief 아이템 구매 메서드
 * @param player 아이템을 구매하는 플레이어
 * @param itemName 구매할 아이템 이름
 * @return 아이템 구매 성공 여부(true - 성공 / false - 실패)
 */
bool Shop::BuyItem(Player* player, const std::string& itemName)
{
    // 상점 재고에서 아이템 찾기
    auto* slot = stock.GetItemSlot(itemName);

    // 재고 확인
    if (!slot || slot->count <= 0)
    {
        std::cout << "상점에 '" << itemName << "' 아이템이 없거나 품절되었습니다." << std::endl;
        return false;
    }

    Item* shopItem = slot->item;
    int price = shopItem->GetPrice();

    // 플레이어 잔고 확인
    if (player->GetMoney() < price)
    {
        std::cout << "골드가 부족합니다! (필요: " << price << "G / 보유: " << player->GetMoney() << "G)" << std::endl;
        return false;
    }

    // 구매 진행
    player->SetMoney(player->GetMoney() - price);
    player->GetInventory().AddItem(new Item(*shopItem), 1);
    stock.UseItem(nullptr, itemName, 1);
    std::cout << "'" << itemName << "'을(를) 구매했습니다! (-" << price << "G)" << std::endl;
    return true;
}

/*
 * @brief 아이템 판매 메서드
 * @param player 아이템을 판매하는 플레이어
 * @param itemName 판매할 아이템 이름
 * @param amount 판매할 아이템 수량
 * @return 아이템 판매 성공 여부(true - 성공 / false - 실패)
 */
bool Shop::SellItem(Player* player, const std::string& itemName, int amount = 1)
{
    // 플레이어 인벤토리에 해당 아이템이 실제로 있는지 확인
    auto* slot = player->GetInventory().GetItemSlot(itemName);

    if (!slot || slot->count < amount)
    {
        std::cout << "판매할 아이템이 없습니다." << std::endl;
        return false;
    }

    // 판매 금액
    int sellPrice = slot->item->GetSellPrice() * amount;


    // 판매 진행
    player->GetInventory().UseItem(nullptr, itemName, amount);
    player->SetMoney(player->GetMoney() + sellPrice);
    stock.AddItem(new Item(*(slot->item)), amount);

    return true;
}
