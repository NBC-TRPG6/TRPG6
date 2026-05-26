#include "IPCManager.h"
#include <iostream>
#include <vector>
#include <functional>

#define LOG_PIPE_NAME L"\\\\.\\pipe\\GameLogPipe"
#define MULTI_PIPE_NAME L"\\\\.\\pipe\\GameMultiPipe"

HANDLE IPCManager::CreatePipeAndProcess(const wchar_t* pipeName, const wchar_t* arg)
{
    HANDLE hPipe = CreateNamedPipeW(
        pipeName, PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 4096, 4096, 0, NULL);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);

        std::wstring cmdLine = std::wstring(exePath) + L" " + arg;
        std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
        cmdBuffer.push_back(L'\0');

        if (!CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        {
            CloseHandle(hPipe);
            return INVALID_HANDLE_VALUE;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        ConnectNamedPipe(hPipe, NULL);
    }
    return hPipe;
}

void IPCManager::InitParentWindows()
{
    hLogPipe = CreatePipeAndProcess(LOG_PIPE_NAME, L"--logviewer");
    hMultiPipe = CreatePipeAndProcess(MULTI_PIPE_NAME, L"--multiviewer");
}

void IPCManager::SendLog(const std::string& log)
{
    if (hLogPipe == INVALID_HANDLE_VALUE) return;
    std::string message = log + "\n";
    DWORD bytesWritten;
    WriteFile(hLogPipe, message.c_str(), (DWORD)message.length(), &bytesWritten, NULL);
}

void IPCManager::SendChat(const std::string& sender, const std::string& msg)
{
    if (hMultiPipe == INVALID_HANDLE_VALUE) return;
    std::string packet = "CHAT|" + sender + "|" + msg + "\n";
    DWORD bytesWritten;
    WriteFile(hMultiPipe, packet.c_str(), (DWORD)packet.length(), &bytesWritten, NULL);
}

void IPCManager::SendPlayerJoin(bool isServer, const std::string& name)
{
    if (hMultiPipe == INVALID_HANDLE_VALUE) return;
    // 패킷 형식: JOIN|SERVER|이름\n
    std::string msg = "JOIN|" + std::string(isServer ? "SERVER" : "CLIENT") + "|" + name + "\n";
    DWORD bytesWritten;
    WriteFile(hMultiPipe, msg.c_str(), (DWORD)msg.length(), &bytesWritten, NULL);
}

void IPCManager::SendPlayerLeave(const std::string& name)
{
    if (hMultiPipe == INVALID_HANDLE_VALUE) return;
    // 패킷 형식: LEAVE|이름\n
    std::string msg = "LEAVE|" + name + "\n";
    DWORD bytesWritten;
    WriteFile(hMultiPipe, msg.c_str(), (DWORD)msg.length(), &bytesWritten, NULL);
}

void IPCManager::Shutdown()
{
    if (hLogPipe != INVALID_HANDLE_VALUE)
    {
        DisconnectNamedPipe(hLogPipe);
        CloseHandle(hLogPipe);
        hLogPipe = INVALID_HANDLE_VALUE;
    }
    if (hMultiPipe != INVALID_HANDLE_VALUE)
    {
        DisconnectNamedPipe(hMultiPipe);
        CloseHandle(hMultiPipe);
        hMultiPipe = INVALID_HANDLE_VALUE;
    }
}

// ==========================================
// 자식 프로세스 루프
// ==========================================

void IPCManager::RunLogViewer()
{
    // 1. 빠른 편집 모드 비활성화 (마우스 클릭으로 인한 메인 루프 블로킹 방지)
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD consoleMode;
    if (GetConsoleMode(hInput, &consoleMode))
    {
        consoleMode &= ~ENABLE_QUICK_EDIT_MODE;
        consoleMode |= ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(hInput, consoleMode);
    }

    // 2. 로그 전용 파이프 연결 (읽기 전용)
    HANDLE hPipe = CreateFileW(
        LOG_PIPE_NAME,
        GENERIC_READ,
        0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        char buffer[1024];
        DWORD bytesRead;
        std::cout << "[시스템 로그 뷰어 시작됨]" << std::endl;
        std::cout << "========================================" << std::endl;

        // 3. 파이프가 끊어질 때까지 무한정 읽어서 출력
        while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            buffer[bytesRead] = '\0';
            std::cout << buffer;
        }

        std::cout << "\n[메인 프로세스와의 연결이 끊어졌습니다. 2초 후 창을 닫습니다.]\n";
        Sleep(2000);
    }
    else
    {
        std::cout << "[에러] 로그 파이프 연결에 실패했습니다. (메인 게임을 먼저 실행해주세요.)" << std::endl;
        Sleep(3000);
    }
}

void IPCManager::RunMultiViewer()
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD consoleMode;
    if (GetConsoleMode(hInput, &consoleMode))
    {
        consoleMode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hInput, consoleMode | ENABLE_EXTENDED_FLAGS);
    }

    HANDLE hPipe = CreateFileW(MULTI_PIPE_NAME, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) return;

    // 접속자 목록 <역할(SERVER/CLIENT), 이름>
    std::vector<std::pair<std::string, std::string>> connectedPlayers;

    // 채팅 로그 <발신자, 내용>
    std::vector<std::pair<std::string, std::string>> chatLogs;
    const int MAX_CHAT_LINES = 15;

    auto RedrawUI = [&]()
        {
            std::string out;
            out.reserve(2048);
            out += "\033[1;1H\033[2J"; // 전체 지우고 커서 최상단 이동

            // --- 상단: 접속자 목록 ---
            out += "==========================================\n";
            out += " [ 현재 접속 중인 플레이어 ]\n";
            out += " 채팅 입장/퇴장: 111 입력\n";
            out += "------------------------------------------\n";

            if (connectedPlayers.empty())
            {
                out += "  (접속자 없음)\n";
            }
            else
            {
                for (const auto& player : connectedPlayers)
                {
                    // 서버는 노란색, 클라이언트는 일반 색상으로 표시
                    std::string roleColor = (player.first == "SERVER") ? "\033[33m" : "\033[37m";
                    out += "  " + roleColor + "[" + player.first + "] " + player.second + "\033[0m\n";
                }
            }

            // --- 하단: 채팅 로그 ---
            out += "==========================================\n";
            out += " [ 채팅 로그 ]\n";
            out += "------------------------------------------\n";

            for (const auto& chat : chatLogs)
            {
                // 이름 해시를 통해 31~36 (Red ~ Cyan) 사이의 고정 색상 부여 (발신자 구분용)
                int colorCode = 31 + (std::hash<std::string>{}(chat.first) % 6);
                out += " \033[" + std::to_string(colorCode) + "m[" + chat.first + "]\033[0m: " + chat.second + "\n";
            }

            // 남은 빈 줄 채우기 (UI 박스 크기 고정)
            int emptyLines = MAX_CHAT_LINES - (int)chatLogs.size();
            for (int i = 0; i < emptyLines; ++i) out += "\n";

            out += "==========================================\n";
            std::cout << out << std::flush;
        };

    // 초기 화면 그리기
    RedrawUI();

    char buffer[2048];
    DWORD bytesRead;

    // 버퍼 찌꺼기를 누적할 변수 (루프 바깥에 선언)
    std::string packetStream = "";

    // ★ 이 파일 안에 ReadFile 루프는 오직 이것 하나만 존재해야 합니다!
    while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
    {
        buffer[bytesRead] = '\0';
        packetStream += buffer; // 읽은 데이터를 스트림에 누적

        // 스트림에서 '\n'을 찾아서 패킷을 하나씩 잘라냄
        size_t pos = 0;
        while ((pos = packetStream.find('\n')) != std::string::npos)
        {
            // 완전한 하나의 패킷 추출
            std::string data = packetStream.substr(0, pos);
            // 처리한 부분은 스트림에서 삭제
            packetStream.erase(0, pos + 1);

            if (data.empty()) continue;

            // --- 여기서부터는 단일 패킷(data) 파싱 ---
            size_t firstDelim = data.find('|');
            if (firstDelim != std::string::npos)
            {
                std::string cmd = data.substr(0, firstDelim);
                size_t secondDelim = data.find('|', firstDelim + 1);

                if (cmd == "JOIN" && secondDelim != std::string::npos)
                {
                    std::string role = data.substr(firstDelim + 1, secondDelim - firstDelim - 1);
                    std::string name = data.substr(secondDelim + 1);

                    bool exists = false;
                    for (const auto& p : connectedPlayers)
                    {
                        if (p.second == name) { exists = true; break; }
                    }
                    if (!exists) connectedPlayers.push_back({ role, name });
                }
                else if (cmd == "LEAVE")
                {
                    std::string name = data.substr(firstDelim + 1);
                    auto it = std::remove_if(connectedPlayers.begin(), connectedPlayers.end(),
                        [&](const std::pair<std::string, std::string>& p) { return p.second == name; });
                    connectedPlayers.erase(it, connectedPlayers.end());
                }
                else if (cmd == "CHAT" && secondDelim != std::string::npos)
                {
                    std::string sender = data.substr(firstDelim + 1, secondDelim - firstDelim - 1);
                    std::string msg = data.substr(secondDelim + 1);

                    chatLogs.push_back({ sender, msg });
                    if (chatLogs.size() > MAX_CHAT_LINES)
                    {
                        chatLogs.erase(chatLogs.begin());
                    }
                }
            }
        }

        // 뭉친 패킷을 모두 처리한 뒤에 화면을 딱 한 번만 갱신
        RedrawUI();
    }
}
