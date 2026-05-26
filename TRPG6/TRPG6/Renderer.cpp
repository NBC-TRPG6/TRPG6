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

std::string Renderer::topStatus = "게임 시작";
std::vector<std::string> Renderer::leftLines;
std::vector<std::string> Renderer::rightLines;
std::vector<std::string> Renderer::screenBuffer;

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

    // 화면 전체 크기만큼 스크린 버퍼 할당
    screenBuffer.assign(SCREEN_HEIGHT, "");
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
    // 1. 기존 DisplayUI처럼 내부 데이터(배열)를 먼저 업데이트합니다.
    if (part == UIPart::Bottom)
    {
        // Bottom인 경우 입력 버퍼를 업데이트합니다. 
        // (주의: DATABASE.h의 currentQuery와 동기화가 필요합니다)
        Client::currentQuery = text;
    }
    else
    {
        // 나머지 영역은 기존 DisplayUI 로직을 재사용하여 배열에 데이터를 담습니다.
        DisplayUI(part, lineIdx, text);
    }

    // 2. 데이터가 갱신되었으므로 전체 화면 버퍼를 다시 그려서 즉시 출력합니다.
    Render();
}

void Renderer::ClearAllCenterLeftUI()
{
    for (int i = 0; i < (int)leftLines.size(); i++)
        Renderer::DisplayUI(UIPart::CenterLeft, i, "");
}
#pragma endregion

void Renderer::Render()
{
    // 1. 프레임 버퍼 조합을 위한 단일 문자열 선언 (동적 할당 최소화를 위해 reserve 사용)
    std::string frameOutput;
    frameOutput.reserve(SCREEN_WIDTH * SCREEN_HEIGHT * 2);

    // 커서를 맨 위로 초기화
    frameOutput += "\033[1;1H";

    // 2. 상단 고정 아스키 이미지 영역
    for (int i = 0; i < reservedAsciiHeight; ++i)
    {
        std::string asciiLine = (i < (int)topAsciiLines.size()) ? topAsciiLines[i] : "";
        frameOutput += CenterText(asciiLine, SCREEN_WIDTH) + "\n";
    }

    // 3. 상태 표시줄 섹션 (3줄)
    frameOutput += std::string(SCREEN_WIDTH, '=') + "\n";

    char topBuffer[256];
    snprintf(topBuffer, sizeof(topBuffer), "%s | 시간: %.1fs",
        topStatus.c_str(), FRAMECOUNT / TARGET_FPS);

    frameOutput += "| " + CenterText(topBuffer, SCREEN_WIDTH - 4) + " |\n";
    frameOutput += std::string(SCREEN_WIDTH, '=') + "\n";

    // 4. 본문 섹션
    int infoWidth = 20;
    int mainWidth = SCREEN_WIDTH - infoWidth - 7;
    int middleHeight = SCREEN_HEIGHT - 7 - reservedAsciiHeight;

    for (int i = 0; i < middleHeight; ++i)
    {
        std::string lStr = (i < (int)leftLines.size()) ? leftLines[i] : "";
        std::string rStr = (i < (int)rightLines.size()) ? rightLines[i] : "";

        char lineBuffer[256];
        snprintf(lineBuffer, sizeof(lineBuffer), "| %s | %s |\n",
            CenterText(lStr, mainWidth).c_str(),
            CenterText(rStr, infoWidth).c_str());

        frameOutput += lineBuffer;
    }

    // 5. 하단 섹션 (3줄)
    frameOutput += std::string(SCREEN_WIDTH, '=') + "\n";

    // \033[K : 줄 끝까지 남아있는 잔상 삭제
    std::string promptTag = Client::CHAT_MODE ? "[채팅]" : "[입력]";
    frameOutput += "\033[K " + promptTag + " : " + inputBuffer + "_\n";

    frameOutput += std::string(SCREEN_WIDTH, '=') + "\n";

    // 6. 완성된 버퍼를 콘솔에 한 번에 출력 (I/O 호출 1회로 단축)
    printf("%s", frameOutput.c_str());
    fflush(stdout);

    // 프레임 카운트 증가
    FRAMECOUNT++;
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
