#include <chrono>
#include <mutex>
#include "GameManager.h"
#include "Renderer.h"
#include "Controller.h"
#include "DATABASE.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::map<std::pair<UIPart, int>, int> Renderer::timedUIMap;

// ==========================================
// [Static Member Definitions]
// ==========================================

std::string Renderer::topStatus = "INITIALIZING...";
std::vector<std::string> Renderer::leftLines;
std::vector<std::string> Renderer::rightLines;

// 상단 아스키 관련 변수 정의
std::vector<std::string> Renderer::topAsciiLines;
int Renderer::reservedAsciiHeight = 0;

#pragma region Animation
void Renderer::DisplayASCIIAnimation()
{
    IGameState* currentState = GameManager::GetInstance().GetCurrentState();
    if (!currentState) return;

    const std::vector<std::vector<std::string>>* targetFrames = nullptr;
    int frameSpeed = 10;

    // 필요시 상태에 따른 애니메이션 프레임 설정 로직 추가
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
    if (len >= width) return text.substr(0, width);

    int leftPad = (width - len) / 2;
    int rightPad = width - len - leftPad;
    return std::string(leftPad, ' ') + text + std::string(rightPad, ' ');
}

// ==========================================
// [Core Implementation]
// ==========================================

void Renderer::Init()
{
    // 아스키 이미지 영역 높이(reservedAsciiHeight)를 반영하여 본문 높이 재계산
    int middleHeight = SCREEN_HEIGHT - 7 - reservedAsciiHeight;
    if (middleHeight < 0) middleHeight = 0;

    leftLines.assign(middleHeight, "");
    rightLines.assign(middleHeight, "");
}

void Renderer::SetTopASCIIImage(const std::vector<std::string>& asciiArt)
{
    topAsciiLines = asciiArt;
    reservedAsciiHeight = (int)asciiArt.size();

    // 아스키 등록 후 본문 사이즈를 다시 맞추기 위해 Init 호출
    Init();
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
    case UIPart::Bottom:
        break;
    }
}

void Renderer::ForceDisplayUI(UIPart part, int lineIdx, const std::string& text)
{
    int targetY = 0, targetX = 0, targetWidth = 0;
    int infoWidth = 20;
    int mainWidth = SCREEN_WIDTH - infoWidth - 7;

    // 아스키 이미지(reservedAsciiHeight)와 상태창(3줄)을 합친 뒤 본문이 시작됨
    int baseMiddleY = 4 + reservedAsciiHeight;

    switch (part)
    {
    case UIPart::Top:
        // 상태 표시줄이 아스키 이미지 아래로 내려왔으므로 Y좌표 수정
        targetY = reservedAsciiHeight + 2;
        targetX = 3; targetWidth = SCREEN_WIDTH - 4;
        break;
    case UIPart::CenterLeft:
        targetY = baseMiddleY + lineIdx;
        targetX = 3;
        targetWidth = mainWidth;
        break;
    case UIPart::CenterRight:
        targetY = baseMiddleY + lineIdx;
        targetX = mainWidth + 6;
        targetWidth = infoWidth;
        break;
    case UIPart::Bottom:
        targetY = SCREEN_HEIGHT - 1;
        targetX = 12;
        targetWidth = SCREEN_WIDTH - 13;
        break;
    }

    printf("\033[%d;%dH%s", targetY, targetX, CenterText(text, targetWidth).c_str());

    // inputBuffer 원래대로 복구 (DATABASE.h 또는 extern 전역 변수로 존재한다고 가정)
    printf("\033[%d;%dH", SCREEN_HEIGHT - 1, 12 + (int)inputBuffer.length());
    fflush(stdout);
}

void Renderer::ClearAllCenterLeftUI()
{
    for (int i = 0; i < (int)leftLines.size(); i++)
        Renderer::DisplayUI(UIPart::CenterLeft, i, "");
}
#pragma endregion

void Renderer::Render()
{
    printf("\033[1;1H");

    // 1. 상단 고정 아스키 이미지 영역 (가장 먼저 출력)
    for (int i = 0; i < reservedAsciiHeight; ++i)
    {
        std::string asciiLine = (i < (int)topAsciiLines.size()) ? topAsciiLines[i] : "";
        printf("%s\n", CenterText(asciiLine, SCREEN_WIDTH).c_str());
    }

    // 2. 상태 표시줄 섹션 (아스키 이미지 아래에 출력, 3줄)
    for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");
    char topBuffer[256];
    snprintf(topBuffer, sizeof(topBuffer), "현재 상태: %s | 시간: %.1fs",
        topStatus.c_str(), FRAMECOUNT / TARGET_FPS);
    printf("| %s |\n", CenterText(topBuffer, SCREEN_WIDTH - 4).c_str());
    for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");

    // 3. 본문 섹션
    int infoWidth = 20;
    int mainWidth = SCREEN_WIDTH - infoWidth - 7;
    int middleHeight = SCREEN_HEIGHT - 7 - reservedAsciiHeight;

    for (int i = 0; i < middleHeight; ++i)
    {
        std::string lStr = (i < (int)leftLines.size()) ? leftLines[i] : "";
        std::string rStr = (i < (int)rightLines.size()) ? rightLines[i] : "";

        printf("| %s | %s |\n",
            CenterText(lStr, mainWidth).c_str(),
            CenterText(rStr, infoWidth).c_str());
    }

    // 4. 하단 섹션 (3줄)
    for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");

    printf("\033[K");
    // inputBuffer 원래대로 복구
    printf(" [INPUT] : %s_\n", inputBuffer.c_str());

    for (int x = 0; x < SCREEN_WIDTH; ++x) printf("="); printf("\n");

    // 누락되었던 프레임 카운트 증가 복구
    FRAMECOUNT++;
    fflush(stdout);
}

void Renderer::DisplayUITimed(UIPart part, int lineIdx, const std::string& text, float durationSeconds)
{
    DisplayUI(part, lineIdx, text);
    int expireFrame = FRAMECOUNT + static_cast<int>(durationSeconds * TARGET_FPS);
    timedUIMap[{part, lineIdx}] = expireFrame;
}

void Renderer::UpdateTimedUI()
{
    auto it = timedUIMap.begin();
    while (it != timedUIMap.end())
    {
        if (FRAMECOUNT >= it->second)
        {
            DisplayUI(it->first.first, it->first.second, "");
            it = timedUIMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
