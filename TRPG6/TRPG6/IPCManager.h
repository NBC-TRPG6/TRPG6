#pragma once
#include <windows.h>
#include <string>
#include <vector>

class IPCManager {
public:
    static IPCManager& GetInstance()
    {
        static IPCManager instance;
        return instance;
    }

    void InitParentWindows();
    void RunLogViewer();      // 자식 1 (단순 로그)
    void RunMultiViewer();    // 자식 2 (멀티 상태 & 채팅)

    // 데이터 전송 인터페이스
    void SendLog(const std::string& log);

    void SendPlayerJoin(bool isServer, const std::string& name);
    void SendPlayerLeave(const std::string& name);
    void SendChat(const std::string& sender, const std::string& msg);

    void Shutdown();

private:
    IPCManager() = default;
    ~IPCManager() { Shutdown(); }

    HANDLE hLogPipe = INVALID_HANDLE_VALUE;
    HANDLE hMultiPipe = INVALID_HANDLE_VALUE;

    HANDLE CreatePipeAndProcess(const wchar_t* pipeName, const wchar_t* arg);
};
