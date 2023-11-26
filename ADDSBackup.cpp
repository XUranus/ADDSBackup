#include "ADDSBackup.h"
#include <Windows.h>
#include <stdio.h>

#include <atlbase.h>
#include <ActiveDS.h>



using namespace std;


int main2()
{
	cout << "Hello CMake." << endl;

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);

	// Create pipes for the standard output of the child process
	HANDLE hChildStdoutRd = INVALID_HANDLE_VALUE;
	HANDLE hChildStdoutWr = INVALID_HANDLE_VALUE;
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!::CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
		std::cerr << "pipe creation error" << std::endl;
		return -1;
	}
	
	// Set the standard output of the child process to the write end of the pipe
	si.hStdError = hChildStdoutWr;
	si.hStdOutput = hChildStdoutWr;
	si.dwFlags |= STARTF_USESTDHANDLES;
	
	std::wstring cmd = L"wbadmin start backup -backupTarget:E:\ -systemState -quiet";

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
		std::cerr << "create process error" << std::endl;
		return -1;
	}

	// Close the write end of the pipe since you will only be reading from it
	::CloseHandle(hChildStdoutWr);

	// Read the output of the child process from the read end of the pipe
	CHAR buffer[4096];
	DWORD bytesRead = 0;
	DWORD bytesWritten = 0;

	while (::ReadFile(hChildStdoutRd, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead != 0) {
		::WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, bytesRead, &bytesWritten, NULL);
	}
	::CloseHandle(hChildStdoutRd);
	return 0;
}


int main()
{
	::CoInitialize(NULL);
	const wchar_t* domainPath = L"LDAP://DC=xuranus,DC=com";
	CComPtr<IADs> pADs;
	HRESULT hr = ADsGetObject(domainPath, IID_IADs, (void**)&pADs);
	if (SUCCEEDED(hr)) {
		CComBSTR domainName, distinguishedName, defaultNamingContext;
		pADs->get_Name(&domainName);
		pADs->get_ADsPath(&distinguishedName);
		std::wcout << "Lhehe" << domainName.m_str << std::endl;
		std::wcout << "Lhaha" << distinguishedName.m_str << std::endl;
	}
	else {
		std::cout << "error " << hr << std::endl;
	}
	::CoUninitialize();
}