#include "IPCLogger.h"
#include <iostream>
#include <vector>

void IPCLogger::InitParent()
{
    // 명시적으로 유니코드 버전(W) 사용
    hPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\GameLogPipe",
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 4096, 4096, 0, NULL);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // 1. 현재 실행 중인 프로그램의 절대 경로를 동적으로 가져옴
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);

        // 2. 인자 합성
        std::wstring cmdLine = std::wstring(exePath) + L" --logviewer";

        // CreateProcessW는 두 번째 인자의 버퍼를 수정할 수 있으므로 배열로 변환
        std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
        cmdBuffer.push_back(L'\0');

        // 3. 자식 프로세스 실행 검증
        if (!CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        {
            // 실행 실패 시 파이프를 즉시 닫고 함수 종료 (메인 루프 멈춤 방지)
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
            return;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // 4. 안전하게 파이프 연결 대기
        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected)
        {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
}

void IPCLogger::RunChild()
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD consoleMode;
    if (GetConsoleMode(hInput, &consoleMode))
    {
        consoleMode &= ~ENABLE_QUICK_EDIT_MODE;
        consoleMode |= ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(hInput, consoleMode);
    }

    // 명시적으로 유니코드 버전(W) 사용
    hPipe = CreateFileW(
        L"\\\\.\\pipe\\GameLogPipe",
        GENERIC_READ,
        0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        char buffer[1024];
        DWORD bytesRead;
        std::cout << "[시스템 로그 뷰어 시작됨]" << std::endl;
        std::cout << "========================================" << std::endl;

        while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            buffer[bytesRead] = '\0';
            std::cout << buffer;
        }
        std::cout << "\n[메인 게임 종료됨. 2초 후 창을 닫습니다.]\n";
        Sleep(2000);
    }
    else
    {
        std::cout << "[에러] 파이프 연결에 실패했습니다. (메인 게임을 먼저 실행해주세요.)" << std::endl;
        Sleep(3000);
    }
}

void IPCLogger::SendLog(const std::string& log)
{
    if (hPipe == INVALID_HANDLE_VALUE) return;
    std::string message = log + "\n";
    DWORD bytesWritten;
    WriteFile(hPipe, message.c_str(), (DWORD)message.length(), &bytesWritten, NULL);
}

void IPCLogger::Shutdown()
{
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
}
