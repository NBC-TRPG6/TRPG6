#pragma once
#include <string>

class IGameState {
public:
    virtual ~IGameState() = default;

    /// <summary>
    /// 상태에 진입할 때 딱 한 번 호출되는 함수
    /// </summary>
    virtual void Enter();

    /// <summary>
    /// 매 프레임마다 한 번 실행됩니다.
	/// int ch는 ENUM으로 관리하세요
    /// </summary>
    /// <param name="ch">전환할 상태 인덱스</param>
    /// <param name="lastCommand">사용자가 입력한 마지막 명령어 문자열입니다.</param>
    virtual void Update(int ch, std::string& lastCommand) = 0;

    /// <summary>
    /// 상태를 빠져나갈 때 딱 한 번 호출되는 함수
    /// </summary>
    virtual void Exit();

};
