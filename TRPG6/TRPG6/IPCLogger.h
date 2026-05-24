// IPCLogger.h
#pragma once
#include <windows.h>
#include <string>

class IPCLogger {
public:
    static IPCLogger& GetInstance()
    {
        static IPCLogger instance;
        return instance;
    }

    void InitParent();
    void RunChild();
    void SendLog(const std::string& log);
    void Shutdown();

private:
    IPCLogger() = default;
    ~IPCLogger() { Shutdown(); }

    HANDLE hPipe = INVALID_HANDLE_VALUE;
};
