#include "GameManager.h"
#include "IGameState.h"

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
void GameManager::SetCurrentState(IGameState* newState)
{
    // 1. 기존 상태가 존재한다면 파괴하기 전에 Exit를 먼저 호출
    if (CURRENT_STATE != nullptr)
    {
        CURRENT_STATE->Exit();
        delete CURRENT_STATE;
    }

    // 2. 새로운 상태로 교체
    CURRENT_STATE = newState;

    // 3. 새 상태가 정상적으로 할당되었다면 Enter 호출
    if (CURRENT_STATE != nullptr)
    {
        CURRENT_STATE->Enter();
    }
}

/// <summary>
/// 현재 게임 상태 가져오기
/// </summary>
/// <returns>현재 게임 상태</returns>	
IGameState* GameManager::GetCurrentState() {
    return CURRENT_STATE;
}

bool GameManager::GetIsGameRunning() const
{
	return IsGameRunning;
}

void GameManager::SetIsGameRunning(bool isRunning)
{
	IsGameRunning = isRunning;
}
