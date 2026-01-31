// windows/runner/security/encryption_winapi.cpp
#include "encryption_winapi.h"
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

// ==================== AES التشفير باستخدام ====================
std::vector<uint8_t> WinCrypt::AESEncrypt(const std::vector<uint8_t>& data, 
                                         const std::string& password) {
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    
    if (!CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, 0)) {
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_NEWKEYSET)) {
            return std::vector<uint8_t>();
        }
    }
    
    // إنشاء hash من الباسوورد
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    if (!CryptHashData(hHash, (BYTE*)password.c_str(), (DWORD)password.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    // اشتقاق المفتاح من الهاش
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    // احتساب حجم البيانات بعد التشفير
    DWORD dataLen = (DWORD)data.size();
    DWORD bufferLen = dataLen;
    
    if (!CryptEncrypt(hKey, 0, TRUE, 0, NULL, &bufferLen, dataLen)) {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    // نسخ البيانات وتشفيرها
    std::vector<uint8_t> encrypted(bufferLen);
    memcpy(encrypted.data(), data.data(), data.size());
    
    DWORD finalLen = (DWORD)data.size();
    if (!CryptEncrypt(hKey, 0, TRUE, 0, encrypted.data(), &finalLen, bufferLen)) {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    encrypted.resize(finalLen);
    
    // تنظيف الموارد
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return encrypted;
}

std::vector<uint8_t> WinCrypt::AESDecrypt(const std::vector<uint8_t>& encrypted, 
                                         const std::string& password) {
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    
    if (!CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, 0)) {
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    if (!CryptHashData(hHash, (BYTE*)password.c_str(), (DWORD)password.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    // نسخ البيانات وفك التشفير
    std::vector<uint8_t> decrypted = encrypted;
    DWORD dataLen = (DWORD)encrypted.size();
    
    if (!CryptDecrypt(hKey, 0, TRUE, 0, decrypted.data(), &dataLen)) {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    decrypted.resize(dataLen);
    
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return decrypted;
}

// ==================== SHA256 هاش ====================
std::string WinCrypt::SHA256Hash(const std::string& data) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return "";
    }
    
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptHashData(hHash, (BYTE*)data.c_str(), (DWORD)data.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    DWORD hashLen = 0;
    DWORD hashSize = sizeof(DWORD);
    
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &hashSize, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    std::vector<BYTE> hash(hashLen);
    
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash.data(), &hashLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    // تحويل إلى نص سداسي عشري
    std::ostringstream oss;
    for (DWORD i = 0; i < hashLen; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return oss.str();
}

// ==================== توليد سلسلة عشوائية ====================
std::string WinCrypt::GenerateRandomString(size_t length) {
    static const char charset[] = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; i++) {
        result += charset[dis(gen)];
    }
    
    return result;
}

std::vector<uint8_t> WinCrypt::GenerateRandomBytes(size_t length) {
    HCRYPTPROV hProv = 0;
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> bytes(length);
    
    if (!CryptGenRandom(hProv, (DWORD)length, bytes.data())) {
        CryptReleaseContext(hProv, 0);
        return std::vector<uint8_t>();
    }
    
    CryptReleaseContext(hProv, 0);
    return bytes;
}

// ==================== التوقيع الرقمي البسيط ====================
std::vector<uint8_t> WinCrypt::GenerateSignature(const std::string& data) {
    std::string hash = SHA256Hash(data);
    std::vector<uint8_t> signature(hash.begin(), hash.end());
    
    // إضافة بعض التعقيد
    for (size_t i = 0; i < signature.size(); i++) {
        signature[i] ^= (i % 256);
        signature[i] += (i * 7) % 256;
    }
    
    return signature;
}

bool WinCrypt::VerifySignature(const std::string& data, 
                              const std::vector<uint8_t>& signature) {
    std::vector<uint8_t> expected = GenerateSignature(data);
    return expected == signature;
}

// ==================== تشفير XOR بسيط ====================
std::string WinCrypt::SimpleEncrypt(const std::string& text, const std::string& key) {
    return XORWithKey(text, key);
}

std::string WinCrypt::SimpleDecrypt(const std::string& encrypted, const std::string& key) {
    return XORWithKey(encrypted, key); // XOR هو نفسه للتشفير وفك التشفير
}

std::string WinCrypt::XORWithKey(const std::string& data, const std::string& key) {
    if (key.empty()) return data;
    
    std::string result = data;
    size_t keyLen = key.length();
    
    for (size_t i = 0; i < data.length(); i++) {
        result[i] = data[i] ^ key[i % keyLen];
        
        // تعقيد إضافي
        result[i] = (result[i] + (i % 256)) % 256;
    }
    
    return result;
}