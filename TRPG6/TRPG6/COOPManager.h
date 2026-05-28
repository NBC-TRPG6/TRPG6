#pragma once
#include <string>
#include <map>
#include <vector>
#include "Packet.h"

struct COOPPlayerInfo {
    std::string name;
    int atk;
    int hp;
    int maxhp;
    PlayerJob job;
    bool isDead;
    bool isReady;
};

class COOPManager {
public:
    static COOPManager& GetInstance() {
        static COOPManager instance;
        return instance;
    }

    bool isChargingPattern = false;

    void Reset();

    // C2S processing
    void SetPlayerReady(const std::string& name, bool isReady);
    void UpdatePlayerStatus(const std::string& name, int atk, int hp, int maxhp, PlayerJob job, bool isDead);
    void OnPlayerAttack(const std::string& sourceName, const std::string& targetName, int amount);
    void OnPlayerBlock(const std::string& sourceName, const std::string& targetName);
    void OnPlayerHeal(const std::string& sourceName, const std::string& targetName, int amount);
    void OnPlayerItem(const std::string& targetName, const std::string& itemName, int amount);

    // S2C processing (for clients)
    void UpdateTurn(const std::string& targetName, int turn);
    void UpdateMonster(const std::string& targetName, int hp);
    void TakeItem(const std::string& targetName, const std::string& itemName);

    // Helpers
    bool IsMyTurn() const;
    PlayerJob GetMyJob() const;

    std::map<std::string, COOPPlayerInfo> players;
    std::string currentBossName;
    int currentBossHp;
    std::string currentTurnPlayer;
    int currentTurnCount;
    std::string currentBlockSource;
    std::string currentBlockTarget;
    std::vector<std::string> turnOrder;

    void NextTurn();

private:
    COOPManager() = default;
    ~COOPManager() = default;
    
    void CheckAllReady();
    void BossAction();
};
