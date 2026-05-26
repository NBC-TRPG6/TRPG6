#pragma once

class ArenaBattleManager {
public:
    static ArenaBattleManager& GetInstance();

    ArenaBattleManager(const ArenaBattleManager&) = delete;
    ArenaBattleManager& operator=(const ArenaBattleManager&) = delete;

    void ResetSession();

private:
    ArenaBattleManager() = default;
    ~ArenaBattleManager() = default;
};
