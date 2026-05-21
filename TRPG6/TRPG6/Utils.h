/*
* 커서 숨기기와 같은 윈도우 API 관련 유틸리티 함수를 모은 헤더 파일
*/

#pragma once
#include <windows.h>

/// <summary>
/// 콘솔창에 커서를 숨깁니다. 게임 시작 시 한 번만 호출하면 됩니다.
/// </summary>
inline void HideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
}
