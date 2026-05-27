#pragma once
#include <string>
#include <vector>
#include <iostream>

class WeaponItem;  // 전방 선언

/// <summary>
/// 웨폰데이터 구조체, 무기의 정보가 들어있습니다.
/// </summary>
struct WeaponData
{
    std::string Name;        // 무기 이름
    std::string BaseName; //무기 원래 이름
    int UpgradeCount;
    int AttackBonus;         // 무기 공격력
    int HPBonus;         // 무기 체력 보너스
    int Price;               // 가격
};


class WeaponTable
{

private:
    WeaponTable() = default;
    ~WeaponTable() = default;

    static const WeaponData weaponTable[30];


public:
    //무기테이블은 하나만있으니 싱글톤 패턴으로 구현
    static WeaponTable& GetInstance()
    {
        static WeaponTable instance;
        return instance;
    }


    WeaponTable(const WeaponTable&) = delete;
    WeaponTable& operator=(const WeaponTable&) = delete;

    const WeaponData* GetWeaponDataByName(const std::string& name) const; //이름으로 무기 데이터 검색
    std::string GetBaseName(WeaponData& weapon) const; // 테이블에 만들어놓은 원래 이름 사용예정
    int GetUpgradeCount(WeaponData& weapon) const; //테이블에 만들어놓은 업그레이드 카운트 사용예정

    // 조합 — MONSTER_PART 2개 → 랜덤 무기
    WeaponItem* Craft(int price1, int price2);

    // 강화 — 같은 무기 2개 → 강화된 무기 (원본 2개 삭제는 SmithState에서 처리)
    WeaponItem* Upgrade(WeaponItem* weapon1, WeaponItem* weapon2);

    WeaponData GetRandomWeapon(int price1, int price2); // Craft에서 사용할 랜덤 무기 생성 함수(구 크래프트)




};
