#pragma once
#include <string>

enum class ItemType
{
    HP_POTION,      // 체력 회복
    ATTACK_BUFF,    // 공격력 일시 증가 (전투용)
};

class Item
{
public:
    // 생성자
    Item(std::string name, ItemType type, int value, int price)
        : name(name), type(type), value(value), price(price) {}

    // 소멸자
    virtual ~Item() {}

    // getter
    std::string GetName() const { return name; }
    ItemType GetType() const { return type; }
    int GetValue() const { return value; }
    int GetPrice() const { return price; }

protected:
    std::string name;           // 아이템 이름
    ItemType type;              // 아이템 타입
    int value;                  // 회복량 혹은 버프 수치
    int price;                  // 아이템 가격
};
