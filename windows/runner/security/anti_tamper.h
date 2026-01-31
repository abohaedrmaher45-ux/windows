// windows/runner/security/anti_tamper.h
#ifndef ANTI_TAMPER_H
#define ANTI_TAMPER_H

#include <string>

class AntiTamper {
public:
    static bool CheckForTampering();
    
    // طرق الكشف المختلفة
    static bool IsDebuggerAttached();
    static bool IsBeingDebugged();
    static bool CheckCodeIntegrity();
    static bool CheckFileModification();
    static bool CheckForeignDLLs();
    
    // إجراءات دفاعية
    static void TriggerDefense();
    static void CleanExit();
    static void CorruptData();
    
private:
    static std::string GetExecutableChecksum();
    static void StoreChecksum();
    static bool ValidateChecksum();
    
    static bool IsKnownDebugger();
    static bool HasBreakpoints();
};

#endif // ANTI_TAMPER_H