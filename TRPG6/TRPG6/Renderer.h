#pragma once
#include <string>
#include <vector>

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
	/// 특정 영역의 UI를 강제로 출력하고 싶을 때 사용합니다.
	/// </summary>
	/// <param name="part">출력할 UI 영역입니다.</param>
	/// <param name="lineIdx">출력할 줄 번호입니다.</param>
	/// <param name="text">출력할 텍스트입니다.</param>
    static void DisplayUI(UIPart part, int lineIdx, const std::string& text);
    static void ForceDisplayUI(UIPart part, int lineIdx, const std::string& text);

	// 편의 기능

	/// <summary>
	/// 아스키 애니메이션을 출력하는 함수입니다.
	/// 현재 상태에 따라 UI의 CenterRight 하단에 아스키 아트를 출력합니다.
	/// </summary>
    static void DisplayASCIIAnimation();


    static void ClearAllCenterLeftUI();

private:
    // UI 데이터 상태 저장소
    static std::string topStatus;
    static std::vector<std::string> leftLines;
    static std::vector<std::string> rightLines;
};
