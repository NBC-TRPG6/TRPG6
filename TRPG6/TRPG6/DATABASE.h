// 게임 내에서 전역적으로 참조되는 데이터를 넣으세요
// 너무 과도하게 사용하면 객체지향을 위반하니 주의할 것

#pragma once
#include <chrono>
#include <filesystem>

// 게임 시스템 설정 =========================================================================
inline double TARGET_FPS = 30;
inline std::chrono::steady_clock::duration FRAME_DURATION =
    std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double, std::milli>(1000.0 / TARGET_FPS));
inline int FRAMECOUNT = 0;
inline const int SCREEN_WIDTH = 64;
inline const int TOP_ASCII_MAX_SIZE = SCREEN_WIDTH / 2;
inline const int SCREEN_HEIGHT = 20 + TOP_ASCII_MAX_SIZE;
inline bool READ_MODE = false; // 쓰기 상태 전환(숫자 이외 입력 가능)

inline const std::filesystem::path ROOT_DIR = std::filesystem::current_path();
inline const std::filesystem::path RESOURCES_DIR = ROOT_DIR / "Resources";

inline void ApplyFrameDurationFromFps(double fps)
{
    if (fps <= 0) fps = 0.1f;
    TARGET_FPS = fps;
	using Ms = std::chrono::duration<double, std::milli>;
    FRAME_DURATION = std::chrono::duration_cast<std::chrono::steady_clock::duration>(Ms(1000.0 / TARGET_FPS));
}

// 게임 내 변수 설정 =======================================================================
enum class EGameState
{
    Start,

    Battle,
    BossBattle,

    Shop,
    Inventory,

    GameOver,
    GameClear,
};

// 네트워크 설정 ===========================================================================
namespace Server
{
    inline int connectedPlayersCount = 1;
    // inline std::vector<Player> playerList;
    // inline int currentTurnIndex = 0;
}

namespace Client
{
    inline bool isServer = false; // 현재 내가 방장(Host)인지 여부
    inline bool CHAT_MODE = false; // 채팅 모드 활성화 여부
    inline std::string playerName = "Unknown";
    inline std::string currentQuery = ""; // 외부 사용 시 메인 루프에서 컨트롤러가 정상적으로 작동안할 수 있습니다.
}
