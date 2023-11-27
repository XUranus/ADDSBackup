#include "ADDSBackup.h"
#include <Windows.h>
#include <stdio.h>

#include <atlbase.h>
#include <ActiveDS.h>

#include <locale>
#include <codecvt>

#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE /* foring using WCHAR on windows */
#endif

using namespace std;
using namespace adds;

namespace {
    const int MAX_BUFFER_SIZE = 4096;
}

std::wstring Utf8ToUtf16(const std::string& str)
{
    using ConvertTypeX = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<ConvertTypeX> converterX;
    std::wstring wstr = converterX.from_bytes(str);
    return wstr;
}

std::string Utf16ToUtf8(const std::wstring& wstr)
{
    using ConvertTypeX = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<ConvertTypeX> converterX;
    return converterX.to_bytes(wstr);
}

static bool Readline(HANDLE hFile, std::string& line)
{
    CHAR buffer[MAX_BUFFER_SIZE];
    DWORD bytesRead = 0;
    DWORD bytesWritten = 0;
    line.clear();
    while (::ReadFile(hFile, buffer, MAX_BUFFER_SIZE - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        // Process the buffer (extract lines)
        char* token = ::strtok(buffer, "\n");
        while (token != NULL) {
            // Process each line (token)
            line = std::string(token);
            token = ::strtok(NULL, "\n");
        }
    }
    return true;
}

WinServerBackupExecutor::WinServerBackupExecutor(
    const std::wstring& wBackupTarget,
    bool systemState,
    OnTextLineOutputFunc onTextLineOutput)
    : m_wBackupTarget(wBackupTarget), m_systemState(systemState), m_onTextLineOutput(onTextLineOutput)
{}

bool WinServerBackupExecutor::Start()
{
    m_execThreadStatus = CMD_EXEC_STATUS::INIT;
    m_execThread = std::thread(&WinServerBackupExecutor::MainThread, this);
    return m_execThreadStatus != CMD_EXEC_STATUS::FAILED;
}

bool WinServerBackupExecutor::Abort()
{
    return false;// TODO
}

WinServerBackupExecutor::~WinServerBackupExecutor()
{
    if (m_execThread.joinable()) {
        m_execThread.join();
    }
}

void WinServerBackupExecutor::MainThread()
{
    STARTUPINFOW si {};
    PROCESS_INFORMATION pi {};
    ::ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);

    // Create pipes for the standard output of the child process
    HANDLE hChildStdoutRd = INVALID_HANDLE_VALUE;
    HANDLE hChildStdoutWr = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES saAttr {};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!::CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
        m_errorCode = ::GetLastError();
        m_execThreadStatus = CMD_EXEC_STATUS::FAILED;
        return;
    }
    
    // Set the standard output of the child process to the write end of the pipe
    si.hStdError = hChildStdoutWr;
    si.hStdOutput = hChildStdoutWr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    std::wstring cmd = GetCmdW();

    // Create the child process
    if (!::CreateProcessW(
        NULL,		// Application Name (use NULL to specify the command in next parameter,
        const_cast<LPWSTR>(cmd.c_str()), // Command line
        NULL,		// Process security attribute
        NULL,		// Thread security attribute
        TRUE,		// Inherit handles
        0,			// Creation flags
        NULL,		// Environment block
        NULL,		// Current directory
        &si,		// STARTUPINFO
        &pi			// PROCESS_INFORMATION
    )) {
        m_errorCode = ::GetLastError();
        m_execThreadStatus = CMD_EXEC_STATUS::FAILED;
        return;
    }

    // Close the write end of the pipe since you will only be reading from it
    ::CloseHandle(hChildStdoutWr);

    // Read the output of the child process from the read end of the pipe
    std::string nextline;
    while (Readline(hChildStdoutRd, nextline)) {
        m_onTextLineOutput(nextline);
    }
    ::CloseHandle(hChildStdoutRd);
    m_execThreadStatus = CMD_EXEC_STATUS::COMPLETED;
    return;
}

std::wstring WinServerBackupExecutor::GetCmdW() const
{
    std::wstring wcmd = L"wbadmin start backup -backupTarget: ";
    wcmd += m_wBackupTarget;
    if (m_systemState) {
        wcmd += L" -systemState ";
    }
    wcmd += L" -quiet";
    return wcmd;
}

int main()
{
    char message[20] = "dwedewdwe";
    char* p = strtok(message, "\n");
    if (p) {
        std::cout << p << std::endl;
    } else {
        std::cout << "not found" << std::endl;
    }
    return 0;
}