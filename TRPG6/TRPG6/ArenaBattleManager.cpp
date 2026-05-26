#include "ArenaBattleManager.h"

ArenaBattleManager& ArenaBattleManager::GetInstance()
{
    static ArenaBattleManager instance;
    return instance;
}

void ArenaBattleManager::ResetSession()
{
}
