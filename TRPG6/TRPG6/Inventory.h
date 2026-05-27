#pragma once
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional> 
#include "Item.h"
#include "Renderer.h"

class Character;

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
    Inventory()
    {
        AddItem(new Item("HP 포션", ItemType::HP_POTION, 50, 15), 3);
        AddItem(new Item("공격력 포션", ItemType::ATTACK_BUFF, 10, 30), 3);
    }

    ~Inventory()
    {
        for (auto& slot : slots)
        {
            delete slot.item;
        }
        slots.clear();
    }

    /*
     * @brief 아이템 추가 메서드
     * @param newItem 새로 추가할 아이템
     * @param amount  추가할 아이템 수량
     */
    void AddItem(T* newItem, int amount = 1)
    {
        if (newItem == nullptr || amount <= 0) { return; }

        auto* slot = GetItemSlot(newItem->GetName());

        // 기존에 있는 아이템일 경우
        if (slot)
        {
            slot->count += amount;
            delete newItem;
            return;
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
    bool UseItem(Character* target, std::string itemName, int amount = 1)
    {
        // 아이템 찾기
        auto* slot = GetItemSlot(itemName);

        // 아이템이 없거나 수량이 부족할 경우
        if (!slot || slot->count < amount)
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 0, "아이템 보유 여부와 개수를 확인해주세요.", 2.0f);
            return false;
        }

        // 효과 적용
        if (target != nullptr)
        {
            for (int i = 0; i < amount; ++i)
            {
                slot->item->Use(target);
            }
        }

        // 수량 관리
        slot->count -= amount;
        if (slot->count <= 0)
        {
            // 포인터로는 바로 erase가 불가능하므로, 삭제를 위해 iterator를 찾음
            auto it = std::find_if(slots.begin(), slots.end(), [&](const auto& s) {
                return s.item->GetName() == itemName;
            });
            
            if (it != slots.end()) {
                delete it->item;
                slots.erase(it);
            }
        }

        return true;
    }

    /// <summary>
    /// 아이템 제거 함수, 위의 수량 관리부분을 따로 빼서 만든 함수입니다.
    /// </summary>
    /// <param name="itemName">아이템이름</param>
    /// <param name="amount">사용 수량</param>
    /// <returns></returns>
    bool RemoveItem(const std::string& itemName, int amount = 1)
    {
        auto* slot = GetItemSlot(itemName);
        if (!slot || slot->count < amount)
            return false;

        slot->count -= amount;
        if (slot->count <= 0)
        {
            auto it = std::find_if(slots.begin(), slots.end(), [&](const auto& s) {
                return s.item->GetName() == itemName;
                });
            if (it != slots.end())
            {
                delete it->item;
                slots.erase(it);
            }
        }
        return true;
    }


    /*
     * @brief 인벤토리 출력 메서드
     * @param formatter 아이템과 수량을 어떻게 출력할지 정의하는 함수
     */
    void PrintAllItems(std::function<void(const T*, int, int)> formatter) const
    {
        // 인벤 비어있는 경우
        if (slots.empty())
        {
            formatter(nullptr, 0, -1);
            return;
        }

        int index = 0;
        for (const auto& slot : slots)
        {
            formatter(slot.item, slot.count, index++);
        }
    }

    /*
     * @brief 이름으로 아이템 슬롯을 찾아 반환하는 메서드
     * @param itemName 찾을 아이템 이름
     * @return 찾으면 슬롯의 포인터, 못 찾으면 nullptr
     */
    InventorySlot<T>* GetItemSlot(const std::string& itemName)
    {
        for (auto& slot : slots)
        {
            if (slot.item->GetName() == itemName)
            {
                return &slot;
            }
        }
        return nullptr;
    }

    const std::vector<InventorySlot<T>>& GetSlots() const { return slots; } 

private:
    std::vector<InventorySlot<T>> slots;
};



