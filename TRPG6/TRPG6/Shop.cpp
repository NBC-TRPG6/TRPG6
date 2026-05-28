#include "Shop.h"
#include "Player.h"
#include "Renderer.h"
#include <iostream>
#include <algorithm>

Shop::Shop()
{
    // 상점 초기 재고 설정(추후에 다른 곳에 옮겨 작성할지도..!!!)
    stock.AddItem(new Item("HP 포션", ItemType::HP_POTION, 18, 15), 27);
    stock.AddItem(new Item("공격력 포션", ItemType::ATTACK_BUFF, 4, 30), 27);
}

/*
 * @brief 상점의 재고 품목 종류 수를 반환하는 메서드
 * @return 재고 품목 종류 수
 */
int Shop::GetStockSize() const
{
    return (int)stock.GetSlots().size();
}

/*
 * @brief 인덱스를 통해 아이템 이름을 반환하는 메서드
 * @param index 가져올 아이템의 인덱스
 * @return 아이템 이름 (인덱스 범위 밖일 경우 빈 문자열)
 */
std::string Shop::GetItemNameByIndex(int index) const
{
    const auto& slots = stock.GetSlots();
    if (index >= 0 && index < (int)slots.size())
    {
        return slots[index].item->GetName();
    }
    return "";
}

/*
 * @brief 인덱스를 통해 아이템 가격을 반환하는 메서드
 * @param index 가져올 아이템의 인덱스
 * @return 아이템 가격 (인덱스 범위 밖일 경우 0)
 */
int Shop::GetItemPriceByIndex(int index) const
{
    const auto& slots = stock.GetSlots();
    if (index >= 0 && index < (int)slots.size())
    {
        return slots[index].item->GetPrice();
    }
    return 0;
}

/*
 * @brief 인덱스를 통해 아이템 재고 수량을 반환하는 메서드
 * @param index 가져올 아이템의 인덱스
 * @return 아이템 재고 수량 (인덱스 범위 밖일 경우 0)
 */
int Shop::GetItemStockByIndex(int index) const
{
    const auto& slots = stock.GetSlots();
    if (index >= 0 && index < (int)slots.size())
    {
        return slots[index].count;
    }
    return 0;
}

/*
 * @brief 상점 재고 출력 메서드
 */
void Shop::ShowStock() const
{
    Renderer::DisplayUI(UIPart::CenterLeft, 0, "[상점 재고 목록]");
    stock.PrintAllItems([](const Item* item, int count, int index)
        {
            if (index == -1)
            {
                Renderer::DisplayUI(UIPart::CenterLeft, 1, "상점에 재고가 없습니다.");
                return;
            }
            std::string info = item->GetName() + " | 가격: " + std::to_string(item->GetPrice()) + "G | 재고: " + std::to_string(count) + "개";
            Renderer::DisplayUI(UIPart::CenterLeft, index + 1, info);
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
    if (!slot || slot->count <= 0) return false;

    Item* shopItem = slot->item;
    int price = shopItem->GetPrice();

    // 플레이어 잔고 확인
    if (player->GetMoney() < price) return false;

    // 구매 진행
    player->SetMoney(player->GetMoney() - price);
    player->GetInventory().AddItem(new Item(*shopItem), 1);
    stock.UseItem(nullptr, itemName, 1);
    return true;
}

/*
 * @brief 아이템 판매 메서드
 * @param player 아이템을 판매하는 플레이어
 * @param itemName 판매할 아이템 이름
 * @param amount 판매할 아이템 수량
 * @return 아이템 판매 성공 여부(true - 성공 / false - 실패)
 */
bool Shop::SellItem(Player* player, const std::string& itemName, int amount)
{
    // 플레이어 인벤토리에 해당 아이템이 실제로 있는지 확인
    auto* slot = player->GetInventory().GetItemSlot(itemName);

    if (!slot || slot->count < amount) return false;

    // 판매 금액
    Item* itemToSell = slot->item;
    int sellPrice = slot->item->GetSellPrice() * amount;

    // 판매 진행
    Item* newItemForStock = new Item(*itemToSell);
    player->GetInventory().UseItem(nullptr, itemName, amount);
    player->SetMoney(player->GetMoney() + sellPrice);
    stock.AddItem(newItemForStock, amount);

    return true;
}
