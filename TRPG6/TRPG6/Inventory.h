#pragma once
#include <vector>
#include <iostream>
#include "Item.h"

// 인벤토리의 각 칸을 표현하는 템플릿 구조체 
template <typename T>
struct InventorySlot
{
    T* item;
    int count;

    InventorySlot(T* item, int count) : item(item), count(count) {}
};

// 인벤토리
template <typename T>
class Inventory
{
public:
    Inventory() {}
    ~Inventory() {}

    /*
     * @brief 아이템 추가 메서드
     * @param newItem 새로 추가할 아이템
     * @param amount  추가할 아이템 수량
     */
    void AddItem(T* newItem, int amount = 1)
    {
        if (newItem == nullptr || amount <= 0) { return; }

        for (auto& slot : slots)
        {
            // 기존에 있는 아이템일 경우
            if (slot.item->GetName() == newItem->GetName())
            {
                slot.count += amount;
                return;
            }
        }

        // 기존에 없는 아이템일 경우
        slots.push_back(InventorySlot<T>(newItem, amount));
    }

    /*
     * @brief 아이템 사용 메서드
     * @param itemName 사용할 아이템 이름
     * @param amount  사용할 아이템 수량
     * @return 아이템 사용 성공 여부(true - 성공 / false - 실패)
     */
    bool UseItem(std::string itemName, int amount = 1)
    {
        for (auto it = slots.begin(); it != slots.end(); ++it)
        {
            if (it->item->GetName() == itemName)
            {
                // 수량만 감소
                if (it->count > amount)
                {
                    it->count -= amount;
                    return true;
                }
                // 사용 후 제거
                else if (it->count == amount)
                {
                    slots.erase(it);
                    return true;
                }
                // 사용 불가
                else
                {
                    std::cout << "아이템 보유 개수를 확인해주세요." << std::endl;
                    return false;
                }
            }
        }
        return false;
    }

    /*
     * @brief 인벤토리 출력 메서드
     */
    void PrintAllItems() const
    {
        // 인벤 비어있는 경우
        if (slots.empty())
        {
            std::cout << "인벤토리가 비어 있습니다." << std::endl;
            return;
        }

        std::cout << "---- 인벤토리 ----" << std::endl;
        for (const auto& slot : slots)
        {
            std::cout << "[" << slot.item->GetName() << "] x" << slot.count << std::endl;
        }
        std::cout << "------------------" << std::endl;
    }

    const std::vector<InventorySlot<T>>& GetSlots() const { return slots; } 

private:
    std::vector<InventorySlot<T>> slots;
};
