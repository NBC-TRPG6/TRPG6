#pragma once
#include <string>


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
    WeaponData GetRandomWeapon(int price1, int price2); // 몬스터가 드랍하는 재료의 가격을 기준으로 무기 랜덤 생성
    std::string GetBaseName(WeaponData& weapon) const; // 테이블에 만들어놓은 원래 이름 사용예정
    int GetUpgradeCount(WeaponData& weapon) const; //테이블에 만들어놓은 업그레이드 카운트 사용예정

    // TODO:: 아이템으로 옮기기 >>
    // WeaponData CombineWeapons(const WeaponData& weapon1, const WeaponData& weapon2); // 무기 합성 기능





};
