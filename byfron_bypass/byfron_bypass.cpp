#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <Psapi.h>

// Function to enable the SE_DEBUG_NAME privilege for a given process and return the target thread ID
std::pair<DWORD, HANDLE> EnableAllPrivilegesForProcess(const wchar_t* targetModuleName) {
    DWORD targetThreadId = 0;
    HANDLE hToken = NULL;

    // Find the RobloxPlayerBeta.exe process by name
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, targetModuleName) == 0) {
                    DWORD processId = processEntry.th32ProcessID;

                    // Attempt to open the process handle
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
                    if (hProcess != NULL) {
                        // Attempt to open the process token
                        if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
                            // Enable all privileges for the process token
                            TOKEN_PRIVILEGES tokenPrivileges;
                            tokenPrivileges.PrivilegeCount = 4;  // Set count to 0 to enable all privileges

                            TCHAR processName[MAX_PATH];
                            if (GetProcessImageFileName(hProcess, processName, MAX_PATH) > 0) {
                                std::wcout << L"The process name for the elevated thread is: " << processName << std::endl;
                            }

                            if (AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
                                // Close the process handle
                                CloseHandle(hProcess);

                                // Take a snapshot of the threads in the specified process
                                HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
                                if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                                    THREADENTRY32 threadEntry = { sizeof(THREADENTRY32) };
                                    if (Thread32First(hThreadSnapshot, &threadEntry)) {
                                        do {
                                            if (threadEntry.th32OwnerProcessID == processId) {
                                                // Optionally, you can add additional checks here to determine the target thread
                                                targetThreadId = threadEntry.th32ThreadID;
                                                break;
                                            }
                                        } while (Thread32Next(hThreadSnapshot, &threadEntry));
                                    }
                                    CloseHandle(hThreadSnapshot);
                                }
                            }
                            else {
                                std::cerr << "Failed to enable all privileges for the process token. Error: " << GetLastError() << std::endl;
                                CloseHandle(hToken);  // Close the token handle in case of failure
                                CloseHandle(hProcess);
                            }
                        }
                        else {
                            std::cerr << "Failed to open process token. Error: " << GetLastError() << std::endl;
                            CloseHandle(hProcess);  // Close the process handle in case of failure
                        }
                    }
                    else {
                        std::cerr << "Failed to open target process. Error: " << GetLastError() << std::endl;
                    }

                    // Break the loop once the target process is found
                    break;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
        else {
            std::cerr << "Process32First failed. Error: " << GetLastError() << std::endl;
        }
        CloseHandle(hSnapshot);
    }
    else {
        std::cerr << "CreateToolhelp32Snapshot failed. Error: " << GetLastError() << std::endl;
    }

    return std::make_pair(targetThreadId, hToken);
}

// Function to retrieve and print the permissions of a thread
void PrintPrivileges(HANDLE hToken) {
    DWORD dwLengthNeeded;
    if (!GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwLengthNeeded) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "Failed to retrieve token privileges. Error: " << GetLastError() << std::endl;
        return;
    }

    PTOKEN_PRIVILEGES tokenPrivileges = reinterpret_cast<PTOKEN_PRIVILEGES>(new BYTE[dwLengthNeeded]);

    if (!GetTokenInformation(hToken, TokenPrivileges, tokenPrivileges, dwLengthNeeded, &dwLengthNeeded)) {
        std::cerr << "Failed to retrieve token privileges. Error: " << GetLastError() << std::endl;
        delete[] tokenPrivileges;
        return;
    }

    std::cout << "Privileges after elevation:" << std::endl;
    for (DWORD i = 0; i < tokenPrivileges->PrivilegeCount; i++) {
        LUID_AND_ATTRIBUTES privilege = tokenPrivileges->Privileges[i];
        DWORD privilegeNameLength = 0;
        LookupPrivilegeName(NULL, &privilege.Luid, NULL, &privilegeNameLength);

        if (privilegeNameLength > 0) {
            std::vector<wchar_t> privilegeName(privilegeNameLength);
            if (LookupPrivilegeName(NULL, &privilege.Luid, privilegeName.data(), &privilegeNameLength)) {
                std::wcout << L"  " << privilegeName.data();
                if (privilege.Attributes & SE_PRIVILEGE_ENABLED) {
                    std::wcout << L" (Enabled)";
                }
                std::wcout << std::endl;

                // Check for specific privileges (e.g., read and write)
                if (wcscmp(privilegeName.data(), L"SeBackupPrivilege") == 0) {
                    std::wcout << L"    (This privilege allows backup access)" << std::endl;
                }
                if (wcscmp(privilegeName.data(), L"SeRestorePrivilege") == 0) {
                    std::wcout << L"    (This privilege allows restore access)" << std::endl;
                }
                // Add more checks for other privileges as needed
            }
        }
    }

    delete[] tokenPrivileges;
}



int main() {
    
    const wchar_t* targetModuleName = L"RobloxPlayerBeta.exe";

    // Enable SE_DEBUG_NAME privilege for the specified module and get the target thread ID
    std::pair<DWORD, HANDLE> result = EnableAllPrivilegesForProcess(targetModuleName);
    DWORD targetThreadId = result.first;
    HANDLE hToken = result.second;

    if (targetThreadId != 0) {
        // You have elevated privileges for the target thread
        // Proceed with your operations on that thread
        std::cout << "Elevated privileges granted for thread with ID: " << targetThreadId << std::endl;
        PrintPrivileges(hToken);
        std::cout << "Bypassed Byfron, (ENABLED ALL THREADS) - you will have to write some code that will hook into the thread that has all permissions enabled" << std::endl;
        std::cout << "Show Some love to Nano for creating this bypass my Discord is N..#5540";
        std::cout << "Discord Server: https://discord.gg/H58pNXsXzP" << std::endl;
    }
    else {
        std::cerr << "Failed to obtain target thread ID or enable privileges." << std::endl;
    }

    if (hToken != NULL) {
        CloseHandle(hToken);  // Close the token handle
    }

    return 0;
}
