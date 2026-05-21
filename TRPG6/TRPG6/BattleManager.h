#include "Character.h"


/// <summary>
/// 전투 상태 관리 이넘입니다.
/// </summary>
enum class EBattleState {
    Locked,        // 전투 불가 (상점 등)
    Ready,       // 전투 시작 가능
    InProgress,  // 전투 중
    Finished     // 전투 종료 직후, 쓸지는 모르겠음
};

class BattleManager {

private:
	EBattleState CurrentBattleState = EBattleState::Locked; // 배틀상태 관리용 변수입니다. 배틀이 가능한 상태인지, 진행 중인지, 종료 직후인지 등을 관리합니다.


public:
    /// <summary>
	/// 배틀 상태를 반환하는 함수입니다.
    /// </summary>
    /// <returns></returns>
    EBattleState GetBattleState() const { return CurrentBattleState; }

    /// <summary>
	/// 배틀이 가능한 상태인지 반환하는 함수입니다. CurrentBattleState가 Ready일 때만 true를 반환합니다.
    /// </summary>
    /// <returns>전투 가능 여부</returns>
    bool CanStartBattle() const { return CurrentBattleState == EBattleState::Ready; }

	bool IsInBattle() const { return CurrentBattleState == EBattleState::InProgress; }

/// <summary>
/// 랜덤한 몬스터를 생성해서 반환합니다.
/// </summary>
/// <returns>플레이어와 전투할 몬스터</returns>
Character RandomMonster();
/// <summary>
/// 두 캐릭터를 넣으면 전투를 진행하는 함수입니다. 전투가 끝날 때까지 반복됩니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
void Battle(Character& player ,Character& monster);

/// <summary>
/// 플레이어를 넣으면 랜덤몬스터 함수와 배틀 함수를 작동해 전투를 시작하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
void StartBattle(Character& player);


/// <summary>
/// 플레이어 의 차례에 작동하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
bool PlayerTurn(Character& player, Character& monster);
/// <summary>
/// 몬스터의 차례에 작동하는 함수입니다.
/// </summary>
/// <param name="player">플레이어 캐릭터</param>
/// <param name="monster">몬스터 캐릭터</param>
bool MonsterTurn(Character& player, Character& monster);
/// <summary>
/// 배틀종료 후 결과 처리 함수입니다.
/// </summary>
/// <param name="character">승리한 캐릭터(플레이어)</param>
void BattleEnd(Character& character);




};





