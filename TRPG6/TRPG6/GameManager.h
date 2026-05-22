// 게임 내 하나만 존재하는 게임 매니저 클래스입니다. 전역적으로 참조해서 개발 편의성을 챙기세요

#pragma once
class IGameState;
class Player;

/// <summary>
/// 그냥 얘는 모든걸 다함 개큼 데이터도 있음 메서드도 있음 매우 대단한 녀석임
/// </summary>
class GameManager {
private:
    GameManager(); // 싱글턴 구현
    IGameState* CURRENT_STATE = nullptr; // 현재 상태
    bool IsGameRunning = true; // 게임 실행중인지 판단
    Player* player = nullptr; // 플레이어 넣을 자리

public:
    ~GameManager(); // 싱글턴 구현
    GameManager(const GameManager&) = delete; // 복사생성자 금지
    void operator=(const GameManager&) = delete; // 대입 연산 금지

    /// <summary>
    /// 실제 GameManager를 가져옵니다.
    /// </summary>
    inline static GameManager& GetInstance() {
        static GameManager Instance; // static 지역 변수로 인스턴스 생성함
        return Instance;
    }

    /// <summary>
    /// 현재 상태를 설정합니다.
    /// 상태 전환 시 반드시 반영해주세요
    /// </summary>
    void SetCurrentState(IGameState* newState);
    IGameState* GetCurrentState();

    void SetPlayer(Player* p); //플레이어 생성 함수
    Player* GetPlayer() const; // 플레이어 가져오는 함수

#pragma region gettedr, setter
    /// <summary>
    /// 현재 게임이 실행중인가?
    /// </summary>
    bool GetIsGameRunning() const;

    /// <summary>
    /// 현재 게임을 실행할지 말지 설정
    /// </summary>
    void SetIsGameRunning(bool isRunning);
#pragma endregion

    /// <summary>
    /// 현재 게임의 FPS를 설정하는 함수입니다.
    /// </summary>
    /// <param name="fps">설정할 FPS 값</param>
    void SetFps(double fps);
};
