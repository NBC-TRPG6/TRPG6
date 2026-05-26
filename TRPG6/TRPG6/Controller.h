// 플레이어 입력을 처리합니다.

#pragma once
#include <string>

/// <summary>
///  버퍼관리용 전역변수
/// </summary>
inline std::string inputBuffer = "";
inline std::string lastCommand = "";

/// <summary>
/// 플레이어	입력을 처리하는 클래스입니다. 매 프레임마다 ProcessInput()이 호출되어야 합니다.
/// </summary>
class Controller {
    public:
		/// <summary>
/// 입력 처리 함수입니다. 매 프레임마다 호출되어야 합니다.
/// </summary>
        void ProcessInput();
};
