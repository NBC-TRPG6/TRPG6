#include "WeaponTable.h"
#include "WeaponItem.h"
#include <random>
#include <algorithm>
#include "IPCManager.h"
const WeaponData WeaponTable::weaponTable[30] = {
    // 검
    {"나무 검",       "검", 0, 3,  2,  100},
    {"구리 검",       "검", 1, 12,  8,  200},
    {"철 검",         "검", 2, 42,  32,  300},
    {"다이아몬드 검", "검", 3, 84, 102, 400},

    {"네더라이트 검", "검", 4, 150, 240, 500},
    // 도끼
    {"나무 도끼",       "도끼", 0, 5,  1,  120},
    {"구리 도끼",       "도끼", 1, 25, 4,  240},
    {"철 도끼",         "도끼", 2, 60, 16,  360},
    {"다이아몬드 도끼", "도끼", 3, 120, 42, 480},

    {"네더라이트 도끼", "도끼", 4, 240, 100, 600},
    // 창
    {"나무 창",       "창", 0, 3,  3, 110},
    {"구리 창",       "창", 1, 18, 18, 220},
    {"철 창",         "창", 2, 47, 47, 330},
    {"다이아몬드 창", "창", 3, 100, 100, 440},
    {"네더라이트 창", "창", 4, 180, 180, 550},
    // 활
    {"나무 활",       "활", 0, 1, 8 , 110},
    {"구리 활",       "활", 1, 4,  40, 220},
    {"철 활",         "활", 2, 16, 120, 330},
    {"다이아몬드 활", "활", 3, 32, 300, 440},

    {"네더라이트 활", "활", 4, 64, 660, 600},
    // 완드
    {"나무 완드",       "완드", 0, 7, 0,  150},
    {"구리 완드",       "완드", 1, 30, 0, 300},
    {"철 완드",         "완드", 2, 80, 0, 450},
    {"다이아몬드 완드", "완드", 3, 150, 0, 600},

    {"네더라이트 완드", "완드", 4, 300, 0, 800},
    // 건틀릿
    {"나무 건틀릿",       "건틀릿", 0, 4,  -6,  100},
    {"구리 건틀릿",       "건틀릿", 1,32,  -32,  120},
    {"철 건틀릿",         "건틀릿", 2, 64,  -64, 140},
    {"다이아몬드 건틀릿", "건틀릿", 3, 128, -128, 160},

    {"네더라이트 건틀릿", "건틀릿", 4, 400, -400, 1000},
};



const WeaponData* WeaponTable::GetWeaponDataByName(const std::string& name) const //이름으로 무기 데이터 검색
{
    for (const auto& weapon : weaponTable)
    {
        if (weapon.Name == name)
            return &weapon;
    }
    //못찾으면 nullptr 반환
    return nullptr;
}

/// <summary>
/// Monster가 드랍하는 재료의 가격을 기준으로 무기를 랜덤하게 생성하는 함수입니다.
/// </summary>
/// <param name="price1">넣을 재료1의 가격</param>
/// <param name="price2">넣을 재료2의 가격</param>
/// <returns>가격을 토대로 생성된 랜덤 무기반환</returns>
WeaponData WeaponTable::GetRandomWeapon(int price1, int price2)
{
    int PriceSum = price1 + price2;
    int MaxUpgrade = 0; // 무기의 최대 가격 설정
    std::mt19937 rng{ std::random_device{}() };

    // 가격 범위에 따라 무기 선택
    if (PriceSum <= 20)
    {
        MaxUpgrade = 0;
    }
    else if (PriceSum <= 100)
    {
        MaxUpgrade = 2;
    }
    else if (PriceSum > 100)
    {
        MaxUpgrade = 3;
    }

    //0~MaxUpgrade 사이의 랜덤한 업그레이드 카운트 생성
    std::uniform_int_distribution<int> upgradeDist(0, MaxUpgrade);
    int randomUpgrade = upgradeDist(rng);

    // 무기 테이블에서 랜덤으로 무기 선택
    std::vector<const WeaponData*> candidates;
    for (const auto& weapon : weaponTable)
    {
        if (weapon.UpgradeCount == randomUpgrade)
            candidates.push_back(&weapon);  // 포인터만 저장
    }
    if (candidates.empty())
        return weaponTable[0];

    std::uniform_int_distribution<int> candidateDist(0, candidates.size() - 1);
    const WeaponData* result = candidates[candidateDist(rng)];
    return *result;  // 마지막에 한 번만 복사

}

WeaponItem* WeaponTable::Craft(int price1, int price2)
{
    WeaponData data = GetRandomWeapon(price1, price2);

    WeaponItem* result = new WeaponItem(
        data.Name, data.BaseName, data.UpgradeCount,
        data.AttackBonus, data.HPBonus, data.Price);

    return result;

}

std::string WeaponTable::GetBaseName(WeaponData& weapon) const
{
    return weapon.BaseName;
}
int WeaponTable::GetUpgradeCount(WeaponData& weapon) const
{
    return weapon.UpgradeCount;
}


/// <summary>
/// 처음 넣은 무기를 베이스, 두번째 무기를 재료로 업그레이드 하는 함수입니다. 
/// </summary>
/// <param name="weapon1"></param>
/// <param name="weapon2"></param>
/// <returns></returns>
WeaponItem* WeaponTable::Upgrade(WeaponItem* weapon1, WeaponItem* weapon2)
{

    if (weapon1->GetUpgradeCount() >= 4 || weapon2->GetUpgradeCount() >= 4) // 이미 최대 강화된 무기는 강화 불가
        return nullptr;

    // 강화 단계 합산 +, 최대 4 클램프
    int newUpgrade = weapon1->GetUpgradeCount() + weapon2->GetUpgradeCount() + 1;
    newUpgrade = std::min<int>(newUpgrade, 4);

    // 새 무기 이름 조합해서 테이블 조회
    std::string baseName = weapon1->GetBaseName();
    std::string newName = "";
    // 무기 테이블에서 baseName과 newUpgrade에 맞는 무기 찾기 (예: "검" + 2 → "철 검")
    for (const auto& w : weaponTable)
    {
        if (w.BaseName == baseName && w.UpgradeCount == newUpgrade)
        {
            newName = w.Name;
            break;
        }
    }

    if (newName.empty())
        return nullptr;

    const WeaponData* data = GetWeaponDataByName(newName);
    if (data == nullptr)
        return nullptr;

    return new WeaponItem(
        data->Name, data->BaseName, data->UpgradeCount,
        data->AttackBonus, data->HPBonus, data->Price);
}

