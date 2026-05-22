// 게임 내에서 전역적으로 참조되는 데이터를 넣으세요
// 너무 과도하게 사용하면 객체지향을 위반하니 주의할 것

#pragma once
#include <chrono>
#include <filesystem>

// 게임 시스템 설정 =========================================================================
inline double TARGET_FPS = 8;
inline std::chrono::steady_clock::duration FRAME_DURATION =
    std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double, std::milli>(1000.0 / TARGET_FPS));
inline int FRAMECOUNT = 0;
inline const int SCREEN_WIDTH = 64;
inline const int TOP_ASCII_MAX_SIZE = 32;
inline const int SCREEN_HEIGHT = 20 + TOP_ASCII_MAX_SIZE;
inline bool READ_MODE = false; // 쓰기 상태 전환(숫자 이외 입력 가능)

// 1. 현재 프로그램이 실행된 위치(Working Directory)를 기준으로 설정
inline const std::filesystem::path ROOT_DIR = std::filesystem::current_path();

// 2. 프로젝트 루트 아래의 Resources 폴더 경로 정의
inline const std::filesystem::path RESOURCES_DIR = ROOT_DIR / "Resources";

inline void ApplyFrameDurationFromFps(double fps)
{
    if (fps <= 0) fps = 0.1f;
    TARGET_FPS = fps;
	using Ms = std::chrono::duration<double, std::milli>;
    FRAME_DURATION = std::chrono::duration_cast<std::chrono::steady_clock::duration>(Ms(1000.0 / TARGET_FPS));
}

// 게임 내 사용 변수 =========================================================================
// 입력버퍼(플레이어가 현재 입력한 문자열)을 저장하는 전역 변수입니다.
// 외부 사용 시 메인 루프에서 컨트롤러가 정상적으로 작동안할 수 있습니다.
inline std::string currentQuery = "";

#pragma region Definiton example
// 다른 스테이지로 넘어갈 때 해당 변수 사용하면 편하게 관리함
// GameManager에서 현재 스테이지 관리하니 GameManager 클래스를 참조할 것
enum class Stage {
	//MainMenu,
	//GameStart,
	//Inventory,
	//CharacterUpgrade,
	//JobSelection,

	//// 배틀 분기
	//Battle,
	//BattleMap,
	//BattleReward,

	//// 포션 제작소 분기
	//AlchemyWorkshop,
	//AlchemyWorkshopShow,
	//AlchemyWorkshopSearchByName,
	//AlchemyWorkshopSearchByIngredient,
	//AlchemyWorkshopDispense,
	//AlchemyWorkshopReturn,

	//GameClear,
	//GameDefeat,
};
#pragma endregion
