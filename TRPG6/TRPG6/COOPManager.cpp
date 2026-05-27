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
}

void COOPManager::SetPlayerReady(const std::string& name, bool isReady) {
    players[name].name = name;
    players[name].isReady = isReady;
    CheckAllReady();
}

void COOPManager::UpdatePlayerStatus(const std::string& name, int atk, int hp, PlayerJob job, bool isDead) {
    // name별로 정보를 맵에 저장
    auto& info = players[name];
    info.name = name;
    info.atk = atk;
    info.hp = hp;
    info.job = job;
    info.isDead = isDead;
    
    // 로컬 플레이어(나 자신)의 정보라면 GameManager의 Player 객체와 동기화 (UI 출력용)
    if (name == Client::playerName) {
        Player* p = GameManager::GetInstance().GetPlayer();
        if (p) {
            p->SetHp(info.hp);
            p->SetAttack(info.atk);
        }
    }

    if (Client::isServer) {
        NetworkManager::GetInstance().BroadcastCOOPUpdateStatus(name, atk, hp, static_cast<int>(job), isDead);
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
    if (!Client::isServer) return;

    std::vector<std::string> aliveTargets;
    for (auto& pair : players) {
        if (!pair.second.isDead) aliveTargets.push_back(pair.first);
    }
    if (aliveTargets.empty()) return;

    std::string target = aliveTargets[rand() % aliveTargets.size()]; // 보스는 살아있는 무작위 1명만 공격합니다.

    if (currentBlockTarget == "ANY" || currentBlockTarget == target)
    {
        target = currentBlockSource;
    }

    // 기존 데미지 계산
    int damage = get_normal_int(COOP_DB::BOSS_DMG_MEAN, COOP_DB::BOSS_DMG_STDDEV) + COOP_DB::BOSS_DMG_BASE;

    // 10% 확률로 2배 데미지 (크리티컬)
    bool isCritical = (rand() % 100) < 10;
    if (isCritical)
    {
        damage *= 2;
        // (선택) 크리티컬 발생 시 로컬 창에 로그 출력
        IPCManager::GetInstance().SendLog("[경고] 보스의 치명적인 공격! (데미지 2배)");
    }

    int currentHp = players[target].hp - damage;
    // ... (이하 기존 로직 동일)
    int currentHp = players[target].hp - damage;
    if (currentHp < 0) currentHp = 0;
    bool isDead = (currentHp == 0);

    UpdatePlayerStatus(target, players[target].atk, currentHp, players[target].job, isDead);
    currentBlockSource = "";
    currentBlockTarget = "";
}

void COOPManager::OnPlayerAttack(const std::string& sourceName, const std::string& targetName, int amount) {
    if (Client::isServer) {
        currentBossHp -= amount;
        if (currentBossHp <= 0) currentBossHp = 0;
        NetworkManager::GetInstance().BroadcastCOOPUpdateMonster(targetName, currentBossHp);
        
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
        NextTurn();
    }
}

void COOPManager::OnPlayerHeal(const std::string& sourceName, const std::string& targetName, int amount) {
    if (Client::isServer) {
        players[targetName].hp += amount;
        NetworkManager::GetInstance().BroadcastCOOPUpdateStatus(targetName, players[targetName].atk, players[targetName].hp, static_cast<int>(players[targetName].job), players[targetName].isDead);
        NextTurn();
    }
}

void COOPManager::OnPlayerItem(const std::string& targetName, const std::string& itemName, int amount) {
    if (Client::isServer) {
        if (itemName == "HP 포션") {
            players[targetName].hp += amount;
        } else if (itemName == "공격력 포션") {
            players[targetName].atk += amount;
        }
        
        UpdatePlayerStatus(targetName, players[targetName].atk, players[targetName].hp, players[targetName].job, players[targetName].isDead);
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
