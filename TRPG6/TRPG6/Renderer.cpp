#include "GameManager.h"
#include "Renderer.h"
#include "Controller.h"
#include "DATABASE.h"

#pragma region Animation
void Renderer::DisplayASCIIAnimation()
{
	IGameState* currentState = GameManager::GetInstance().GetCurrentState();
	if (!currentState) return;

	const std::vector<std::vector<std::string>>* targetFrames = nullptr;
	int frameSpeed = 10;

	// UI용 아스키 데이터 예시
	//if (dynamic_cast<BattleMapState*>(currentState)) {
	//    static std::vector<std::vector<std::string>> portal = {
	//        { "                ", "     .---.      ", "   /  . .  \\    ", "   \\  . .  /    ", "     '---'      ", "                " },
	//        { "                ", "     .---.      ", "   /  / \\  \\    ", "   \\  \\ /  /    ", "     '---'      ", "                " },
	//        { "                ", "     .---.      ", "   /  - -  \\    ", "   \\  - -  /    ", "     '---'      ", "                " },
	//        { "      ...       ", "   . :::: .     ", "  . ::::::: .   ", "   . :::: .     ", "      '''       ", "                " },
	//        { "      ...       ", "   . :::::: .   ", "  . :::::::: .  ", "   . :::::: .   ", "      '''       ", "                " },
	//        { "      ...       ", "   . :::: .     ", "  . ::::::: .   ", "   . :::: .     ", "      '''       ", "                " }
	//    };
	//    targetFrames = &portal;
	//    frameSpeed = 6;
	//}

	if (targetFrames == nullptr || targetFrames->empty()) return;

	int startIdx = 7;
	int lastIdx = (int)rightLines.size() - 1;
	int availableHeight = lastIdx - startIdx + 1;

	int currentFrame = (FRAMECOUNT / frameSpeed) % targetFrames->size();
	const auto& art = (*targetFrames)[currentFrame];

	DisplayUI(UIPart::CenterRight, 6, "");

	for (int i = 0; i < availableHeight; ++i)
	{
		int targetLine = startIdx + i;
		if (i < (int)art.size())
		{
			DisplayUI(UIPart::CenterRight, targetLine, art[i]);
		}
		else
		{
			DisplayUI(UIPart::CenterRight, targetLine, "");
		}
	}
}
#pragma endregion


// ==========================================
// [Internal Helpers]
// ==========================================

static std::string CenterText(const std::string& text, int width)
{
	int len = (int)text.length();
	// 한글 등 멀티바이트 처리가 필요한 경우 추가 로직이 필요하나, 
	// 여기서는 기본 너비 절단 및 패딩만 수행
	if (len >= width) return text.substr(0, width);

	int leftPad = (width - len) / 2;
	int rightPad = width - len - leftPad;
	return std::string(leftPad, ' ') + text + std::string(rightPad, ' ');
}

// ==========================================
// [Static Member Definitions]
// ==========================================

std::string Renderer::topStatus = "INITIALIZING...";
std::vector<std::string> Renderer::leftLines;
std::vector<std::string> Renderer::rightLines;

// ==========================================
// [Core Implementation]
// ==========================================

void Renderer::Init()
{
	// SCREEN_HEIGHT를 참고하여 본문 높이 계산 (상단 4줄, 하단 3줄 제외)
	int middleHeight = SCREEN_HEIGHT - 7;
	if (middleHeight < 0) middleHeight = 0;

	leftLines.assign(middleHeight, "");
	rightLines.assign(middleHeight, "");
}



#pragma region Print
void Renderer::DisplayUI(UIPart part, int lineIdx, const std::string& text)
{
	switch (part)
	{
	case UIPart::Top:
		topStatus = text;
		break;
	case UIPart::CenterLeft:
		if (lineIdx >= 0 && lineIdx < (int)leftLines.size()) leftLines[lineIdx] = text;
		break;
	case UIPart::CenterRight:
		if (lineIdx >= 0 && lineIdx < (int)rightLines.size()) rightLines[lineIdx] = text;
		break;
		// 유저 입력부라서 특별한 경우가 아니라면 사용하지 말 것
	case UIPart::Bottom:
		break;
	}
}

/// <summary>
/// 특정 영역의 UI를 강제로 출력하고 싶을 때 사용합니다.
/// main.cpp의 렌더링 영역과 별개로 출력이 바로 반영됩니다.
/// </summary>
/// <param name="part">출력할 UI 영역입니다.</param>
/// <param name="lineIdx">출력할 줄 번호입니다.</param>
/// <param name="text">출력할 텍스트입니다.</param>
void Renderer::ForceDisplayUI(UIPart part, int lineIdx, const std::string& text)
{
	int targetY = 0, targetX = 0, targetWidth = 0;
	int infoWidth = 20;
	int mainWidth = SCREEN_WIDTH - infoWidth - 7;

	switch (part)
	{
	case UIPart::Top:
		targetY = 2; targetX = 3; targetWidth = SCREEN_WIDTH - 4;
		break;
	case UIPart::CenterLeft:
		targetY = 4 + lineIdx; // 상단 3줄 이후 첫 번째 본문 줄
		targetX = 3;
		targetWidth = mainWidth;
		break;
	case UIPart::CenterRight:
		targetY = 4 + lineIdx;
		targetX = mainWidth + 6;
		targetWidth = infoWidth;
		break;
	case UIPart::Bottom:
		targetY = SCREEN_HEIGHT - 1;
		targetX = 12;
		targetWidth = SCREEN_WIDTH - 13;
		break;
	}

	// 특정 영역만 덮어쓰기 (기둥 좌표를 건드리지 않음)
	printf("\033[%d;%dH%s", targetY, targetX, CenterText(text, targetWidth).c_str());

	// 커서를 입력창 위치로 복구
	printf("\033[%d;%dH", SCREEN_HEIGHT - 1, 12 + (int)inputBuffer.length());
	fflush(stdout);
}

/// <summary>
/// CenterLeftUI 영역을 초기화할 때 사용합니다.
/// </summary>
void Renderer::ClearAllCenterLeftUI()
{
	for (int i = 0; i < 13; i++)
		Renderer::DisplayUI(UIPart::CenterLeft, i, "");
}
#pragma endregion

void Renderer::Render()
{
	// 0. 커서 원점 복귀
	printf("\033[1;1H");

	// 1. 상단 섹션 (3줄)
	for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");
	char topBuffer[256];
	snprintf(topBuffer, sizeof(topBuffer), "STATUS: %s | Time: %ds",
		topStatus.c_str(), FRAMECOUNT / TARGET_FPS);
	printf("| %s |\n", CenterText(topBuffer, SCREEN_WIDTH - 4).c_str());
	for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");

	// 2. 본문 섹션 (기둥 유지)
	int infoWidth = 20;
	int mainWidth = SCREEN_WIDTH - infoWidth - 7;
	int middleHeight = SCREEN_HEIGHT - 7; // 상단(3) + 하단(3) + 구분선(1) 제외한 본문 높이

	for (int i = 0; i < middleHeight; ++i)
	{
		// 내용이 없어도 기둥(|)은 출력함
		std::string lStr = (i < (int)leftLines.size()) ? leftLines[i] : "";
		std::string rStr = (i < (int)rightLines.size()) ? rightLines[i] : "";

		// 본문 기둥 구조: | [Main] | [Info] |
		printf("| %s | %s |\n",
			CenterText(lStr, mainWidth).c_str(),
			CenterText(rStr, infoWidth).c_str());
	}

	// 3. 하단 섹션 (3줄)
	for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");

	// 입력창 (\033[K로 이전 잔상 지우되 기둥은 다시 그림)
	printf("\033[K");
	printf(" [INPUT] : %s_\n", inputBuffer.c_str());

	for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");

	FRAMECOUNT++;
	fflush(stdout);
}
