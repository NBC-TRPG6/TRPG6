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
    // 보스 관련 설정 (파티 평균 공격력 500, 5인 15~20턴 하드코어 기준)
    inline constexpr int BOSS_MAX_HP = 35000;     // 보스 최대 체력 대폭 상향
    inline constexpr int BOSS_DMG_MEAN = 300;     // 보스 공격력 정규분포 평균
    inline constexpr int BOSS_DMG_STDDEV = 200;   // 보스 공격력 정규분포 표준편차 (분산 극대화)
    inline constexpr int BOSS_DMG_BASE = 300;     // 보스 확정 최저 데미지

    // 직업별 추가 스탯 보너스 (배율 적용 전 Base 값)
    inline constexpr int TANKER_BONUS_HP = 100;   // 탱커 추가 체력 
    inline constexpr int DEALER_BONUS_ATK = 20;   // 딜러(기본) 추가 공격력

    // 힐러 힐량 정규분포 설정 (힐량 상향 조정)
    inline constexpr int HEALER_HEAL_MEAN = 55;   // 힐량 평균 (만렙 시 평균 330)
    inline constexpr int HEALER_HEAL_STDDEV = 15; // 힐량 표준편차 (만렙 시 표준편차 90)

    // 플레이어 레벨 비례 배율 (예: 50레벨일 때 100 + 300 = 400% 적용)
    inline constexpr int STAT_MULTIPLIER_PERCENT_PER_LEVEL = 30;
}
