#pragma once
//기본 라이브러리 헤더
#include <random>

//커스텀 라이브러리 헤더
#include "Player.h"
#include "Monster.h"
#include "Renderer.h"
#include "Item.h"


/// <summary>
/// 전투 상태 관리 이넘입니다.
/// </summary>
enum class EBattleState {
    Locked,        // 전투 불가 (상점 등)
    Ready,       // 전투 시작 가능
    InProgress,  // 전투 중
};

class BattleManager {

    //변수 모음집
private:

    EBattleState CurrentBattleState = EBattleState::Locked; // 배틀상태 관리용 변수입니다. 배틀이 가능한 상태인지, 진행 중인지 등을 관리합니다.
    std::mt19937 rng{ std::random_device{}() };// random_device는 시드로 사용할 수 있는 난수 생성기입니다. mt19937은 Mersenne Twister 알고리즘을 사용하는 난수 생성기입니다.
    bool isBoss; //보스면 true
    //플레이어의 원래 공격력을 OriginalPlayerAttack 에 저장합니다.
    int OriginalPlayerAttack = 0;
    bool isPlayerTurn = false;
    bool isNIMO = false;
    bool NimoDefeated = false;
    Monster currentMonster; // 현재 전투 중인 몬스터를 저장하는 변수입니다.

public:

#pragma region CurrentBattleState &Getters/Setters

    /// <summary>
    /// 배틀 상태를 반환하는 함수입니다.
    /// </summary>
    /// <returns></returns>
    EBattleState GetBattleState() const { return CurrentBattleState; }

    /// <summary>
    /// 배틀이 가능한 상태인지 반환하는 함수입니다. CurrentBattleState가 Ready일 때만 true를 반환합니다.
    /// </summary>
    /// <returns>전투 가능 여부</returns>
    bool GetCanStartBattle() const { return CurrentBattleState == EBattleState::Ready; }

    /// <summary>
    /// 전투중인지 확인하는 함수입니다. CurrentBattleState가 InProgress일 때만 true를 반환합니다.
    /// </summary>
    /// <returns>전투 가능 여부</returns>
    bool GetIsInBattle() const { return CurrentBattleState == EBattleState::InProgress; }

    /// <summary>
    /// 바깥에서 배틀 상태를 설정하는 함수입니다. EBattleState 타입의 값을 넣으면 CurrentBattleState를  변경합니다.
    /// </summary>
    /// <param name="newState">Locked,Ready,InProgress</param>
    void SetBattleState(EBattleState newState) { CurrentBattleState = newState; }

#pragma endregion


#pragma region BattleFunctions

    /// <summary>
    /// 두 캐릭터를 넣으면 전투를 진행하는 함수입니다. 전투가 끝날 때까지 반복됩니다.
    /// </summary>
    /// <param name="player">플레이어 캐릭터</param>
    /// <param name="monster">몬스터 캐릭터</param>
    void Battle(Player* player);


    /// <summary>
    /// 플레이어를 넣으면 랜덤몬스터 함수와 배틀 함수를 작동해 전투를 시작하는 함수입니다.
    /// </summary>
    /// <param name="player">플레이어 캐릭터</param>
    void StartBattle(Player* player);

    /// <summary>
    /// 플레이어 의 차례에 작동하는 함수입니다.
    /// </summary>
    /// <param name="player">플레이어 캐릭터</param>
    /// <param name="monster">몬스터 캐릭터</param>
    void PlayerTurn(Player* player, Monster& monster);

    /// <summary>
    /// 몬스터의 차례에 작동하는 함수입니다.
    /// </summary>
    /// <param name="player">플레이어 캐릭터</param>
    /// <param name="monster">몬스터 캐릭터</param>
    void MonsterTurn(Player* player, Monster& monster);

    /// <summary>
    /// 배틀종료 후 결과 처리 함수입니다.
    /// </summary>
    /// <param name="player">보상을 받을 플레이어</param>
    /// <param name="monster">처치한 몬스터(돈/아이템 제공)</param>
    void BattleEnd(Player* player);


    Monster GetCurrentMonster() const { return currentMonster; }
#pragma endregion

};





