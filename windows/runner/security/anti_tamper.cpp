// windows/runner/security/anti_tamper.cpp
#include "anti_tamper.h"
#include "encryption_winapi.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "psapi.lib")

bool AntiTamper::CheckForTampering() {
    // 1. فحص التصحيح
    if (IsDebuggerAttached()) return true;
    if (IsBeingDebugged()) return true;
    
    // 2. فحص نقاط التوقف
    if (HasBreakpoints()) return true;
    
    // 3. فحص سلامة الملف
    if (!ValidateChecksum()) return true;
    
    // 4. فحص المكتبات الخارجية
    if (CheckForeignDLLs()) return true;
    
    return false;
}

bool AntiTamper::IsDebuggerAttached() {
    // الطريقة المباشرة
    return ::IsDebuggerPresent() == TRUE;
}

bool AntiTamper::IsBeingDebugged() {
    // فحص BeingDebugged flag في PEB
    bool result = false;
    
    __try {
        // PEB موجود في fs:[0x30] على x86 أو gs:[0x60] على x64
    #ifdef _WIN64
        PPEB pPeb = (PPEB)__readgsqword(0x60);
    #else
        PPEB pPeb = (PPEB)__readfsdword(0x30);
    #endif
        
        if (pPeb->BeingDebugged) {
            result = true;
        }
        
        // فحص NtGlobalFlag
        if (pPeb->NtGlobalFlag & 0x70) {
            result = true;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        result = false;
    }
    
    return result;
}

bool AntiTamper::HasBreakpoints() {
    // عناوين دالات مهمة للفحص
    void* addresses[] = {
        (void*)&CheckForTampering,
        (void*)&IsDebuggerAttached,
        (void*)&WinCrypt::SHA256Hash
    };
    
    for (void* addr : addresses) {
        // قراءة البايت الأول من الدالة
        BYTE firstByte = *(BYTE*)addr;
        
        // 0xCC هو breakpoint (INT 3)
        if (firstByte == 0xCC) {
            return true;
        }
    }
    
    return false;
}

std::string AntiTamper::GetExecutableChecksum() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    
    std::ifstream file(exePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    // قراءة أول 1MB من الملف
    const size_t chunkSize = 1024 * 1024;
    std::vector<char> buffer(chunkSize);
    
    file.read(buffer.data(), chunkSize);
    size_t bytesRead = file.gcount();
    
    file.close();
    
    std::string content(buffer.data(), bytesRead);
    return WinCrypt::SHA256Hash(content);
}

void AntiTamper::StoreChecksum() {
    std::string checksum = GetExecutableChecksum();
    if (checksum.empty()) return;
    
    HKEY hKey;
    const char* keyPath = "Software\\MaherApp";
    const char* valueName = "FileChecksum";
    
    if (RegCreateKeyExA(HKEY_CURRENT_USER, keyPath, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                       &hKey, NULL) == ERROR_SUCCESS) {
        
        RegSetValueExA(hKey, valueName, 0, REG_SZ,
                      (const BYTE*)checksum.c_str(),
                      (DWORD)checksum.length() + 1);
        
        // تخزين تاريخ التثبيت أيضاً
        SYSTEMTIME st;
        GetLocalTime(&st);
        char dateStr[20];
        sprintf_s(dateStr, "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
        RegSetValueExA(hKey, "InstallDate", 0, REG_SZ,
                      (const BYTE*)dateStr, (DWORD)strlen(dateStr) + 1);
        
        RegCloseKey(hKey);
    }
}

bool AntiTamper::ValidateChecksum() {
    // الحصول على التوقيع المخزن
    HKEY hKey;
    const char* keyPath = "Software\\MaherApp";
    const char* valueName = "FileChecksum";
    
    char storedChecksum[65] = {0};
    DWORD bufferSize = sizeof(storedChecksum);
    
    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        // أول تشغيل، نقوم بتخزين التوقيع
        StoreChecksum();
        return true;
    }
    
    if (RegQueryValueExA(hKey, valueName, NULL, NULL,
                        (LPBYTE)storedChecksum, &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        StoreChecksum();
        return true;
    }
    
    RegCloseKey(hKey);
    
    // حساب التوقيع الحالي والمقارنة
    std::string currentChecksum = GetExecutableChecksum();
    return currentChecksum == storedChecksum;
}

bool AntiTamper::CheckForeignDLLs() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    
    if (!Module32First(hSnapshot, &me32)) {
        CloseHandle(hSnapshot);
        return false;
    }
    
    do {
        std::string moduleName = me32.szModule;
        std::transform(moduleName.begin(), moduleName.end(),
                      moduleName.begin(), ::tolower);
        
        // قائمة بالمكتبات المشبوهة
        const char* suspicious[] = {
            "cheatengine", "x64dbg", "x32dbg", "ollydbg",
            "ida", "wireshark", "fiddler", "processhacker",
            "hack", "inject", "hook", "debug"
        };
        
        for (const char* suspiciousName : suspicious) {
            if (moduleName.find(suspiciousName) != std::string::npos) {
                CloseHandle(hSnapshot);
                return true;
            }
        }
        
    } while (Module32Next(hSnapshot, &me32));
    
    CloseHandle(hSnapshot);
    return false;
}

void AntiTamper::TriggerDefense() {
    if (CheckForTampering()) {
        // 1. تسجيل الحدث
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\MaherApp",
                         0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            
            DWORD tamperCount = 0;
            DWORD bufferSize = sizeof(DWORD);
            
            RegQueryValueExA(hKey, "TamperCount", NULL, NULL,
                            (LPBYTE)&tamperCount, &bufferSize);
            
            tamperCount++;
            RegSetValueExA(hKey, "TamperCount", 0, REG_DWORD,
                          (const BYTE*)&tamperCount, sizeof(DWORD));
            
            RegCloseKey(hKey);
        }
        
        // 2. إذا كانت المحاولات كثيرة، ننفذ إجراء أقوى
        if (rand() % 100 < 30) { // 30% فرصة للإغلاق
            CleanExit();
        }
    }
}

void AntiTamper::CleanExit() {
    ExitProcess(0);
}

void AntiTamper::CorruptData() {
    // محاكاة خطأ في الذاكرة
    volatile int* ptr = nullptr;
    *ptr = 0xDEADBEEF;
}