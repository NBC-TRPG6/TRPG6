// Utils.h
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <vector>
#include <string>
#include <cwchar>
#include <random>
#include <cmath>

// stb_image.h 인클루드 (구현체 정의는 Renderer.cpp에서 수행)
#include "stb_image.h"
#include "DATABASE.h"

inline void HideCursor()
{
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
}

/// <summary>
/// 이미지를 로드하여 아스키 아트 문자열 배열로 반환합니다.
/// </summary>
inline std::vector<std::string> LoadImageAsASCII(const char* filepath)
{
    int targetWidth = SCREEN_WIDTH, targetHeight = TOP_ASCII_MAX_SIZE;
    std::vector<std::string> asciiScreen(targetHeight, std::string(targetWidth, ' '));
    const std::string ASCII_CHARS = " .:-=+*#%@";

    int imgWidth, imgHeight, channels;
    // 1채널(Grayscale)로 강제 로드
    unsigned char* imgData = stbi_load(filepath, &imgWidth, &imgHeight, &channels, 1);

    if (imgData == nullptr)
    {
        // 로드 실패 시 에러 메시지가 담긴 빈 배열 반환
        asciiScreen[0] = "IMAGE LOAD FAILED";

        return asciiScreen;
    }

    for (int y = 0; y < targetHeight; ++y)
    {
        for (int x = 0; x < targetWidth; ++x)
        {
            int srcX = (x * imgWidth) / targetWidth;
            int srcY = (y * imgHeight) / targetHeight;

            int pixelIndex = srcY * imgWidth + srcX;
            unsigned char brightness = imgData[pixelIndex];

            int charIndex = (brightness * (ASCII_CHARS.size() - 1)) / 255;
            asciiScreen[y][x] = ASCII_CHARS[charIndex];
        }
    }
    stbi_image_free(imgData);
    return asciiScreen;
}



inline std::vector<std::string> LoadImageAsASCIIColor(const char* filepath)
{
    int targetWidth = SCREEN_WIDTH, targetHeight = TOP_ASCII_MAX_SIZE;
    std::vector<std::string> asciiScreen(targetHeight, "");
    const std::string ASCII_CHARS = " .:-=+*#%@";
    int imgWidth, imgHeight, channels;
    // 3채널(RGB)로 로드
    unsigned char* imgData = stbi_load(filepath, &imgWidth, &imgHeight, &channels, 3);
    if (imgData == nullptr)
    {
        asciiScreen[0] = "IMAGE LOAD FAILED";
        return asciiScreen;
    }
    for (int y = 0; y < targetHeight; ++y)
    {
        std::string row = "";
        for (int x = 0; x < targetWidth; ++x)
        {
            int srcX = (x * imgWidth) / targetWidth;
            int srcY = (y * imgHeight) / targetHeight;
            int pixelIndex = (srcY * imgWidth + srcX) * 3;
            unsigned char r = imgData[pixelIndex];
            unsigned char g = imgData[pixelIndex + 1];
            unsigned char b = imgData[pixelIndex + 2];
            unsigned char brightness = (unsigned char)(0.299f * r + 0.587f * g + 0.114f * b);
            int charIndex = (brightness * (ASCII_CHARS.size() - 1)) / 255;
            // r, g, b 실제 값으로 색상 적용
            row += "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
            row += ASCII_CHARS[charIndex];
        }
        asciiScreen[y] = row + "\033[0m";
    }
    stbi_image_free(imgData);
    return asciiScreen;
}


// 정규분포 샘플링
// 확률참고
// mean +- 1sigma: 약 68.27%
// mean +- 2sigma: 약 95.45%
// mean +- 3sigma: 약 99.73%
static inline int get_normal_int(double mean, double sigma)
{
    // 난수 생성기와 시드는 스레드별로 한 번만 초기화되도록 static thread_local 사용
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());

    // 전달받은 평균과 시그마로 정규분포 객체 생성 (비용이 적으므로 매번 생성해도 무방)
    std::normal_distribution<double> dist(mean, sigma);

    // 난수 추출 후 반올림하여 정수형으로 반환
    return static_cast<int>(std::round(dist(gen)));
}
