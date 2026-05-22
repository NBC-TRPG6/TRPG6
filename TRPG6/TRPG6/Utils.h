// Utils.h
#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <cwchar>

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

