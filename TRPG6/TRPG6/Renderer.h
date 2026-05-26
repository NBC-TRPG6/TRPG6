#pragma once
#include <string>
#include <map>
#include <vector>

// 전방선언

/// <summary>
/// UI 출력 영역을 태깅합니다.
/// Top: 상단 상태 표시줄 (1줄)
/// CenterLeft: 본문 왼쪽 (최대 13줄)
/// CenterRight: 본문 오른쪽 (최대 13줄)
/// Bottom: 입력창 (1줄, 특수 취급, ForceDisplayUI로만 출력 가능)
/// </summary>
enum class UIPart { Top, CenterLeft, CenterRight, Bottom };

class Renderer {
public:
    // 초기화 및 프레임 루프용 인터페이스
    static void Init();
    static void Render();

	// 출력 기능
	
	/// <summary>
	/// 특정 영역의 UI를 출력하고 싶을 때 사용합니다.
	/// </summary>
	/// <param name="part">출력할 UI 영역입니다.</param>
	/// <param name="lineIdx">출력할 줄 번호입니다.</param>
	/// <param name="text">출력할 텍스트입니다.</param>
    static void DisplayUI(UIPart part, int lineIdx, const std::string& text);

	/// <summary>
    /// 되도록 사용하지 마세요
    /// 프레임 계산에 오류가 발생할 수 있습니다.
	/// 특정 영역의 UI를 강제로 출력하고 싶을 때 사용합니다.
	/// </summary>
	/// <param name="part">출력할 UI 영역입니다.</param>
	/// <param name="lineIdx">출력할 줄 번호입니다.</param>
	/// <param name="text">출력할 텍스트입니다.</param>
    static void ForceDisplayUI(UIPart part, int lineIdx, const std::string& text);

    /// <summary>
    /// 몇초 동안 특정 영역의 UI를 출력하고 싶을 때 사용합니다.
    /// </summary>
    /// <param name="part">출력할 UI 영역입니다.</param>
    /// <param name="lineIdx">출력할 줄 번호입니다.</param>
    /// <param name="text">출력할 텍스트입니다.</param>
    /// <param name="durationSeconds">출력 시간</param>
    static void DisplayUITimed(UIPart part, int lineIdx, const std::string& text, float durationSeconds);

    /// <summary>
    /// 매 프레임 호출되어 예약된 UI를 검사하고 지우는 함수
    /// </summary>
    static void UpdateTimedUI();

    /// <summary>
    /// 외부 프롬프트에 로그를 출력합니다.
    /// </summary>
    /// <param name="text">출력할 로그 텍스트</param>
    static void DisplayLog(const std::string& text);

	// 편의 기능

	/// <summary>
	/// 아스키 애니메이션을 출력하는 함수입니다.
	/// 현재 상태에 따라 UI의 CenterRight 하단에 아스키 아트를 출력합니다.
	/// </summary>
    static void DisplayASCIIAnimation();

    /// <summary>
    /// CenterLeftUI 영역을 초기화할 때 사용합니다.
    /// </summary>
    static void ClearAllCenterLeftUI();

    /// <summary>
    /// 게임 화면 상단에 아스키 아트를 출력하는 함수입니다.
    /// </summary>
    /// <param name="asciiArt">출력할 아스키 아트입니다.</param>
    static void SetTopASCIIImage(const std::vector<std::string>& asciiArt);

private:
    // UI 데이터 상태 저장소
    static std::string topStatus;
    static std::vector<std::string> leftLines;
    static std::vector<std::string> rightLines;
    static std::vector<std::string> screenBuffer;

    // 비동기 처리용 기록 저장
    static std::map<std::pair<UIPart, int>, int> timedUIMap;

    // 상단 아스키 영역 데이터 및 예약된 높이
    static std::vector<std::string> topAsciiLines;
    static int reservedAsciiHeight;
};
