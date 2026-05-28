#include "COOPManager.h"
#include "NetworkManager.h"
#include "IPCManager.h"
#include "GameManager.h"
#include "DATABASE.h"
#include "Player.h"
#include "Utils.h"
#include "Item.h"

void COOPManager::Reset() {
    players.clear();
    currentBossName = "이승재";
    currentBossHp = COOP_DB::BOSS_MAX_HP;
    currentTurnPlayer = "";
    currentTurnCount = 0;
    currentBlockSource = "";
    currentBlockTarget = "";
    turnOrder.clear();
    isChargingPattern = false;
}

void COOPManager::SetPlayerReady(const std::string& name, bool isReady) {
    players[name].name = name;
    players[name].isReady = isReady;
    CheckAllReady();
}

void COOPManager::UpdatePlayerStatus(const std::string& name, int atk, int hp, int maxhp, PlayerJob job, bool isDead) {
    // name별로 정보를 맵에 저장
    auto& info = players[name];
    info.name = name;
    info.atk = atk;
    info.hp = hp;
    info.maxhp = maxhp;
    info.job = job;
    info.isDead = isDead;
    
    // 로컬 플레이어(나 자신)의 정보라면 GameManager의 Player 객체와 동기화 (UI 출력용)
    if (name == Client::playerName) {
        Player* p = GameManager::GetInstance().GetPlayer();
        if (p) {
            info.maxhp = p->GetMaxHp();
            p->SetHp(info.hp);
            p->SetAttack(info.atk);
        }
    }

    if (Client::isServer) {
        NetworkManager::GetInstance().BroadcastCOOPUpdateStatus(name, atk, hp, maxhp, static_cast<int>(job), isDead);
    }
}

void COOPManager::CheckAllReady() {
    if (!Client::isServer) return;
    
    // 방장 본인(1) + 연결된 클라이언트 수 만큼 모두 대기실에 정보를 보냈는지 대기합니다.
    size_t expectedCount = NetworkManager::GetInstance().GetConnectedClientCount() + 1;
    if (players.size() < expectedCount) return;

    bool allReady = true;
    if (players.empty()) allReady = false;
    for (auto& pair : players) {
        if (!pair.second.isReady) {
            allReady = false;
            break;
        }
    }
    
    if (allReady) {
        NetworkManager::GetInstance().ApplySyncedStateChange(EGameState::COOPBattle);
        NetworkManager::GetInstance().BroadcastCOOPUpdateMonster(currentBossName, currentBossHp);
        
        turnOrder.clear();
        for (auto& pair : players) {
            turnOrder.push_back(pair.first);
        }
        currentTurnCount = 1; 
        if (!turnOrder.empty()) {
            // 첫 번째 살아있는 플레이어 찾기
            for (const auto& name : turnOrder) {
                if (!players[name].isDead) { currentTurnPlayer = name; break; }
            }
            NetworkManager::GetInstance().BroadcastCOOPUpdateTurn(currentTurnPlayer, currentTurnCount);
        }
    }
}

void COOPManager::NextTurn() {
    if (!Client::isServer) return;
    if (turnOrder.empty()) return;
    
    int startIndex = 0;
    auto it = std::find(turnOrder.begin(), turnOrder.end(), currentTurnPlayer);
    if (it != turnOrder.end()) {
        startIndex = (int)std::distance(turnOrder.begin(), it);
    }

    bool foundNext = false;
    // 모든 플레이어를 순회하며 다음 살아있는 플레이어 검색
    for (int i = 1; i <= (int)turnOrder.size(); ++i) {
        int nextIdx = (startIndex + i) % (int)turnOrder.size();
        
        // 한 바퀴 돌아서 다시 처음으로 오면 보스가 행동함
        if (nextIdx == 0) {
            BossAction();
            currentTurnCount++;
        }

        if (!players[turnOrder[nextIdx]].isDead) {
            currentTurnPlayer = turnOrder[nextIdx];
            foundNext = true;
            break;
        }
    }

    if (!foundNext) {
        NetworkManager::GetInstance().ApplySyncedStateChange(EGameState::GameOver);
        return;
    }

    NetworkManager::GetInstance().BroadcastCOOPUpdateTurn(currentTurnPlayer, currentTurnCount);
}

void COOPManager::BossAction()
{
    if (!Client::isServer) return; //

    std::vector<std::string> aliveTargets;
    for (auto& pair : players)
    {
        if (!pair.second.isDead) aliveTargets.push_back(pair.first); //
    }
    if (aliveTargets.empty()) return; //

    // 1. 만약 보스가 이전 턴에서 패턴을 준비(차징) 중이었다면 발동
    if (isChargingPattern)
    {
        isChargingPattern = false; // 발동 후 상태 초기화

        std::vector<std::string> nonBlockers;
        for (const auto& name : aliveTargets)
        {
            if (name != currentBlockSource)
            {
                nonBlockers.push_back(name);
            }
        }

        if (!nonBlockers.empty())
        {
            IPCManager::GetInstance().SendLog("[경고] 보스 이승재의 광역 마킹 패턴이 발동되었습니다!");

            for (const auto& target : nonBlockers)
            {
                int damage = get_normal_int(COOP_DB::BOSS_PATTERN_DMG_MEAN, COOP_DB::BOSS_PATTERN_DMG_STDDEV) + COOP_DB::BOSS_DMG_BASE;

                NetworkManager::GetInstance().SendChatPacket("[레이드]", "보스 " + currentBossName + "가 도발을 하지 않은 " + target + "를 공격했습니다! (피해: " + std::to_string(damage) + ")");

                int currentHp = players[target].hp - damage;
                if (currentHp < 0) currentHp = 0;
                bool isDead = (currentHp == 0);

                UpdatePlayerStatus(target, players[target].atk, currentHp, players[target].maxhp, players[target].job, isDead);
            }
        }
        else
        {
            // 전원 도발 상태거나 탱커만 남았을 경우의 예외 처리 (패턴 무효화)
            NetworkManager::GetInstance().SendChatPacket("[레이드]", "모든 플레이어가 도발 효과로 보호받아 보스의 광역 공격이 빗나갔습니다!");
        }

        currentBlockSource = "";
        currentBlockTarget = "";
        return; // 발동 후 턴 종료
    }

    // 2. 특수 패턴을 충전 중이 아닐 때, 10% 확률로 전조 현상 발생 (차징 턴)
    bool isSpecialPattern = (rand() % 100) < COOP_DB::BOSS_PATTERN_CHANCE;

    if (isSpecialPattern)
    {
        isChargingPattern = true; // 차징 상태 진입

        IPCManager::GetInstance().SendLog("[경고] 보스 이승재가 크게 숨을 들이마시며 광역 공격을 준비합니다!");
        NetworkManager::GetInstance().SendChatPacket("[레이드]", "보스 " + currentBossName + "가 모두를 노려봅니다... 다음 턴에 강력한 광역 공격이 예상됩니다! (도발 필요)");

        currentBlockSource = "";
        currentBlockTarget = "";
        return; // 전조 현상만 보여주고 턴을 넘김 (이 턴에는 공격 안 함)
    }

    // 3. 차징도 발동도 아닐 경우 기존 일반 공격 로직 수행
    std::string target = aliveTargets[rand() % aliveTargets.size()]; //

    if (currentBlockTarget == "ANY" || currentBlockTarget == target) //
    {
        target = currentBlockSource; //
    }

    int damage = get_normal_int(COOP_DB::BOSS_DMG_MEAN, COOP_DB::BOSS_DMG_STDDEV) + COOP_DB::BOSS_DMG_BASE; //

    bool isCritical = (rand() % 100) < 10; //
    if (isCritical) //
    {
        damage *= 2; //
        IPCManager::GetInstance().SendLog("[경고] 보스의 치명적인 공격! (데미지 2배)"); //
        NetworkManager::GetInstance().SendChatPacket("[레이드]", "보스의 치명적인 공격! " + target + "에게 " + std::to_string(damage) + "의 피해를 입혔습니다!"); //
    }
    else //
    {
        NetworkManager::GetInstance().SendChatPacket("[레이드]", "보스 " + currentBossName + "가 " + target + "를 공격했습니다! (피해: " + std::to_string(damage) + ")"); //
    }

    int currentHp = players[target].hp - damage; //
    if (currentHp < 0) currentHp = 0; //
    bool isDead = (currentHp == 0); //

    UpdatePlayerStatus(target, players[target].atk, currentHp, players[target].maxhp, players[target].job, isDead);
    currentBlockSource = ""; //
    currentBlockTarget = ""; //
}

void COOPManager::OnPlayerAttack(const std::string& sourceName, const std::string& targetName, int amount) {
    if (Client::isServer) {
        currentBossHp -= amount;
        if (currentBossHp <= 0) currentBossHp = 0;
        NetworkManager::GetInstance().BroadcastCOOPUpdateMonster(targetName, currentBossHp);
        
        NetworkManager::GetInstance().SendChatPacket("[레이드]", sourceName + "이(가) 보스에게 " + std::to_string(amount) + "의 피해를 입혔습니다.");

        if (currentBossHp <= 0) {
            NetworkManager::GetInstance().ApplySyncedStateChange(EGameState::COOPReward);
        } else {
            NextTurn();
        }
    }
}

void COOPManager::OnPlayerBlock(const std::string& sourceName, const std::string& targetName) {
    if (Client::isServer) {
        currentBlockSource = sourceName;
        currentBlockTarget = targetName;
        NetworkManager::GetInstance().SendChatPacket("[레이드]", sourceName + "이(가) 도발을 사용했습니다.");
        NextTurn();
    }
}

void COOPManager::OnPlayerHeal(const std::string& sourceName, const std::string& targetName, int amount) {
    if (Client::isServer) {
        players[targetName].hp += amount;
        NetworkManager::GetInstance().BroadcastCOOPUpdateStatus(targetName, players[targetName].atk, players[targetName].hp, players[targetName].maxhp, static_cast<int>(players[targetName].job), players[targetName].isDead);
        NetworkManager::GetInstance().SendChatPacket("[레이드]", sourceName + "이(가) " + targetName + "에게 " + std::to_string(amount) + "만큼 힐을 했습니다.");
        NextTurn();
    }
}

void COOPManager::OnPlayerItem(const std::string& targetName, const std::string& itemName, int amount) {
    if (Client::isServer) {
        if (itemName == "HP 포션") {
            int maxHp = players[targetName].maxhp;
            players[targetName].hp = min(players[targetName].hp + maxHp * amount / 100, maxHp);
        } else if (itemName == "공격력 포션") {
            int currentAtk = players[targetName].atk;

            // 2. 넘어온 amount(4)를 퍼센트로 환산하여 증가량을 계산하고 반올림합니다.
            int bonusAtk = (int)std::round(currentAtk * amount / 100.0f);

            // 3. 계산된 버프 수치를 기존 공격력에 더해줍니다.
            players[targetName].atk = currentAtk + bonusAtk;
        }
        
        UpdatePlayerStatus(targetName, players[targetName].atk, players[targetName].hp, players[targetName].maxhp, players[targetName].job, players[targetName].isDead);
        NextTurn();
    }
}

void COOPManager::UpdateTurn(const std::string& targetName, int turn) {
    currentTurnPlayer = targetName;
    currentTurnCount = turn;
}

void COOPManager::UpdateMonster(const std::string& targetName, int hp) {
    currentBossName = targetName;
    currentBossHp = hp;
}

void COOPManager::TakeItem(const std::string& targetName, const std::string& itemName) {
    if (Client::playerName == targetName) {
        Player* p = GameManager::GetInstance().GetPlayer();
        if (p) p->GetInventory().AddItem(new Item(itemName, ItemType::MONSTER_PART, 100, 100));
    }
}

bool COOPManager::IsMyTurn() const {
    return Client::playerName == currentTurnPlayer;
}

PlayerJob COOPManager::GetMyJob() const {
    auto it = players.find(Client::playerName);
    if (it != players.end()) {
        return it->second.job;
    }
    return PlayerJob::None;
}
