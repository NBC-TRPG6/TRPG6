#include "GameManager.h"
#include "IGameState.h"
#include "Player.h"

GameManager::GameManager() {
    // 생성자
    // 게임에서 제일 빨리 실행되는 부분입니다.(UIManager 제외)
}

GameManager::~GameManager() {
    // 파괴자
    delete CURRENT_STATE;
    CURRENT_STATE = nullptr;
}

/// <summary>
/// 현재 게임 상태 설정
/// </summary>
/// <param name="newState">새로운 게임 상태</param>
void GameManager::SetCurrentState(IGameState* newState) {
    if (CURRENT_STATE != nullptr) {
        delete CURRENT_STATE;
    }
    CURRENT_STATE = newState;
}

/// <summary>
/// 현재 게임 상태 가져오기
/// </summary>
/// <returns>현재 게임 상태</returns>	
IGameState* GameManager::GetCurrentState() {
    return CURRENT_STATE;
}

void GameManager::SetPlayer(Player* p)
{
    player = p;
}

Player* GameManager::GetPlayer() const
{
    return player;
}

bool GameManager::GetIsGameRunning() const
{
    return IsGameRunning;
}

void GameManager::SetIsGameRunning(bool isRunning)
{
    IsGameRunning = isRunning;
}
