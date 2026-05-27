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
inline const int SCREEN_WIDTH = 80;
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

    ArenaReady,
    ArenaLobby,
    ArenaBattle,
    ArenaWait,
    ArenaResult,

    COOPReady,
    COOPSelectJob,
    COOPBattle,
    COOPReward
};

// 아레나 네트워크 상수 ======================================================================
inline constexpr int MAX_ARENA_ITEM_SLOTS = 14;
inline constexpr int MAX_ARENA_PLAYERS = 8;

// 네트워크 설정 ===========================================================================
namespace Server
{
    inline int connectedPlayersCount = 1;
}

namespace Client
{
    inline bool isServer = false;
    inline bool CHAT_MODE = false;
    inline std::string playerName = "Unknown";
    inline std::string currentQuery = "";
}

namespace COOP_DB
{
    // 보스 관련 설정 (4~6인 기준 15~20턴 장기전 목표)
    inline constexpr int BOSS_MAX_HP = 12000;
    inline constexpr int BOSS_DMG_MEAN = 150;
    inline constexpr int BOSS_DMG_STDDEV = 50;
    inline constexpr int BOSS_DMG_BASE = 280;

    // 직업별 추가 스탯 보너스 (Base)
    inline constexpr int TANKER_BONUS_HP = 100;
    inline constexpr int DEALER_BONUS_ATK = 10;

    // 힐러 힐량 정규분포 설정 (Base)
    inline constexpr int HEALER_HEAL_MEAN = 15;
    inline constexpr int HEALER_HEAL_STDDEV = 4;

    // [수정] float 대신 순수 정수 백분율(%) 사용 (예: 10 = 레벨당 10% 증가)
    inline constexpr int STAT_MULTIPLIER_PERCENT_PER_LEVEL = 10;
}
