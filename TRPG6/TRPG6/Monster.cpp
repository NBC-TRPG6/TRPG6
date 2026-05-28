#include "Monster.h"
#include "IPCManager.h"
#include <random>

std::string Monster::MonsterNames[11]{
   "고블린", "오크", "트롤", "스켈레톤", "좀비",
   "뱀파이어", "늑대인간", "거미", "슬라임", "산적", "대래래래래곤~~~"
};
std::string Monster::MonsterImageNames[11]{
   "goblin", "orc", "troll", "skeleton", "zombie",
    "vampire", "werewolf", "spider", "slime", "bandit", "dragon"
};
std::string Monster::MonsterItems[11]{
   "고블린 코", "오크 귀", "트롤 발톱", "스켈레톤 뼈", "좀비 살점",
   "뱀파이어 송곳니", "늑대인간 털", "거미 다리", "슬라임 젤리", "산적 칼", "대래래래래곤~~~ 심장"
};

std::string Monster::RandomName()
{
    int num = 0 + rand() % (9 - 0 + 1);
    return MonsterNames[num];
}

int Monster::RandomState(int pLevel, int min, int max)
{
    return pLevel * (min + rand() % (max - min + 1));
}

int Monster::RandomHp(int pLevel, float multi)
{
    return RandomState(pLevel, int(20 * multi), int(30 * multi));
}

int Monster::RandomAttack(int pLevel, float multi)
{
    return RandomState(pLevel, int(5 * multi), int(10 * multi));
}

int Monster::RandomMoney(int pLevel, float multi)
{
    return RandomState(pLevel, int(5 * multi), int(10 * multi));
}

std::string Monster::WhoAmI()
{
    return "Monster";
}


Monster* Monster::ResetState(int playerLevel)
{
    Name = RandomName();
    Hp = RandomHp(playerLevel);
    Attack = RandomAttack(playerLevel);
    Money = RandomMoney(playerLevel);
    itemMoney = Money / 2;
    return this;
}

std::string Monster::GetImageName()
{
    for (int i = 0; i < 11; ++i) {
        if (MonsterNames[i] == Name) {
            return MonsterImageNames[i];
        }
    }
    return "default_image"; // 일치하는 이름이 없을 경우 기본 이미지 반환
}

Item* Monster::DropItem(int percent)
{
    int num = 0;
    if (Name == "대래래래래곤~~~")
    {
        num = 10;
    }
    else {

        for (int i = 0; i < 10; ++i) {
            if (Monster::MonsterNames[i] == Name) {
                num = i; // 일치하는 string을 찾으면 즉시 인덱스 반환
            }
        }
    }

    IPCManager::GetInstance().SendLog("몬스터 이름: " + Name + ", 드롭 아이템: " + std::to_string(num) + ", 드롭 확률: " + std::to_string(percent) + "%");

    std::mt19937 rng{ std::random_device{}() };// 랜덤생성기
    std::uniform_int_distribution<int> distPercent(0, 99);
    bool itemDropped = distPercent(rng) < percent; // percent% 확률로 아이템 드롭
    std::uniform_real_distribution<float> distMoney(0.8f, 2.0f);
    itemMoney = int(itemMoney * distMoney(rng)); // 아이템 가격에 0.8~2.0 사이의 랜덤한 배율 적용
    if (itemDropped)
    {
        Item* droppedItem = new Item(MonsterItems[num], ItemType::MONSTER_PART, 50, itemMoney);
        IPCManager::GetInstance().SendLog("아이템 드롭 성공: " + droppedItem->GetName());
        return droppedItem;
    }

    return nullptr;
}

