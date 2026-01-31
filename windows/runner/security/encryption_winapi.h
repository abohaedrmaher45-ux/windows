// windows/runner/security/encryption_winapi.h
#ifndef ENCRYPTION_WINAPI_H
#define ENCRYPTION_WINAPI_H

#include <windows.h>
#include <wincrypt.h>
#include <string>
#include <vector>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")

class WinCrypt {
public:
    // التشفير وفك التشفير
    static std::vector<uint8_t> AESEncrypt(const std::vector<uint8_t>& data, 
                                          const std::string& password);
    static std::vector<uint8_t> AESDecrypt(const std::vector<uint8_t>& encrypted, 
                                          const std::string& password);
    
    // التجزئة (Hash)
    static std::string SHA256Hash(const std::string& data);
    
    // توليد بيانات عشوائية
    static std::string GenerateRandomString(size_t length);
    static std::vector<uint8_t> GenerateRandomBytes(size_t length);
    
    // التوقيع الرقمي البسيط
    static std::vector<uint8_t> GenerateSignature(const std::string& data);
    static bool VerifySignature(const std::string& data, 
                               const std::vector<uint8_t>& signature);
    
    // التشفير البسيط للنصوص
    static std::string SimpleEncrypt(const std::string& text, const std::string& key);
    static std::string SimpleDecrypt(const std::string& encrypted, const std::string& key);
    
private:
    // دالة مساعدة لاشتقاق المفتاح
    static bool DeriveKey(HCRYPTPROV hProv, HCRYPTHASH hHash, HCRYPTKEY* phKey);
    
    // تشفير XOR بسيط
    static std::string XORWithKey(const std::string& data, const std::string& key);
};

#endif // ENCRYPTION_WINAPI_H