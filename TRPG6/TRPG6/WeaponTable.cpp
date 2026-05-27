#include "WeaponTable.h"
#include <random>
#include <vector>
const WeaponData WeaponTable::weaponTable[30] = {
    // 검
    {"나무 검",       "검", 0, 3,  2,  100},
    {"구리 검",       "검", 1, 5,  4,  200},
    {"철 검",         "검", 2, 8,  8,  300},
    {"다이아몬드 검", "검", 3, 10, 16, 400},
    {"네더라이트 검", "검", 4, 15, 24, 500},
    // 도끼
    {"나무 도끼",       "도끼", 0, 5,  1,  120},
    {"구리 도끼",       "도끼", 1, 8, 2,  240},
    {"철 도끼",         "도끼", 2, 13, 4,  360},
    {"다이아몬드 도끼", "도끼", 3, 17, 6, 480},
    {"네더라이트 도끼", "도끼", 4, 23, 12, 600},
    // 창
    {"나무 창",       "창", 0, 3,  3, 110},
    {"구리 창",       "창", 1, 5, 5, 220},
    {"철 창",         "창", 2, 9, 9, 330},
    {"다이아몬드 창", "창", 3, 13, 13, 440},
    {"네더라이트 창", "창", 4, 17, 17, 550},
    // 활
    {"나무 활",       "활", 0, 1, 8 , 110},
    {"구리 활",       "활", 1, 2,  12, 220},
    {"철 활",         "활", 2, 3, 25, 330},
    {"다이아몬드 활", "활", 3, 4, 51, 440},
    {"네더라이트 활", "활", 4, 6, 80, 600},
    // 완드
    {"나무 완드",       "완드", 0, 7, 0,  150},
    {"구리 완드",       "완드", 1, 11, 0, 300},
    {"철 완드",         "완드", 2, 16, 0, 450},
    {"다이아몬드 완드", "완드", 3, 22, 0, 600},
    {"네더라이트 완드", "완드", 4, 30, 0, 800},
    // 건틀릿
    {"나무 건틀릿",       "건틀릿", 0, 4,  -6,  100},
    {"구리 건틀릿",       "건틀릿", 1, 6,  -8,  120},
    {"철 건틀릿",         "건틀릿", 2, 8,  -10, 140},
    {"다이아몬드 건틀릿", "건틀릿", 3, 10, -12, 160},
    {"네더라이트 건틀릿", "건틀릿", 4, 50, -50, 1000},
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
    else if (PriceSum >= 200)
    {
        MaxUpgrade = 3;
    }

    //0~MaxUpgrade 사이의 랜덤한 업그레이드 카운트 생성
    std::uniform_int_distribution<int> upgradeDist(0, MaxUpgrade);
    int randomUpgrade = upgradeDist(rng);

    // 무기 테이블에서 랜덤으로 무기 선택
    std::vector<WeaponData> candidates;
    for (const auto& weapon : weaponTable)
    {
        if (weapon.UpgradeCount == randomUpgrade)
        {
            //업그레이드 수치가 같은 6종 무기 추가
            candidates.push_back(weapon);
        }
    }
    if (candidates.empty())
    {
        return weaponTable[0]; // 후보가 없으면 기본 무기 반환
    }
    //6종의 무기중 랜덤으로 하나 선택해서 리턴
    std::uniform_int_distribution<int> candidateDist(0, candidates.size() - 1);
    return candidates[candidateDist(rng)];

}

std::string WeaponTable::GetBaseName(WeaponData& weapon) const
{
    return weapon.BaseName;
}
int WeaponTable::GetUpgradeCount(WeaponData& weapon) const
{
    return weapon.UpgradeCount;
}


