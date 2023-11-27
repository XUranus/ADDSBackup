#pragma once
#pragma comment(lib, "activeds.lib")
#pragma comment(lib, "adsiid.lib")

#include <iostream>
#include <thread>
#include <functional>

namespace adds {

enum CMD_EXEC_STATUS {
    INIT = 0,
    RUNNING = 1,
    COMPLETED = 2,
    FAILED = 3,
    ABORTED = 4
};

class WinServerBackupExecutor {
public:
    using OnTextLineOutputFunc = std::function<void(std::string)>;

    WinServerBackupExecutor(
        const std::wstring& wBackupTarget,
        bool systemState,
        OnTextLineOutputFunc onTextLineOutput);
    
    ~WinServerBackupExecutor();

    bool Start();

    bool Abort();

private:
    void MainThread();

    std::wstring GetCmdW() const;

private:
    std::wstring            m_wBackupTarget;
    bool                    m_systemState { true };
    OnTextLineOutputFunc    m_onTextLineOutput { [&](std::string) {} };

    std::thread             m_execThread;
    CMD_EXEC_STATUS         m_execThreadStatus { CMD_EXEC_STATUS::INIT };
    int                     m_errorCode { 0 };
};

}