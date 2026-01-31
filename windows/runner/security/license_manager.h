// windows/runner/security/license_manager.h
#ifndef LICENSE_MANAGER_H
#define LICENSE_MANAGER_H

#include <string>
#include <vector>
#include <ctime>

class LicenseManager {
public:
    // Singleton pattern
    static LicenseManager& GetInstance();
    
    // الواجهة الرئيسية
    bool Initialize();
    bool CheckLicense();
    bool Activate(const std::string& licenseKey);
    void Deactivate();
    
    // معلومات الترخيص
    std::string GetLicenseType() const;
    int GetRemainingDays() const;
    std::string GetExpiryDate() const;
    bool IsTrial() const;
    bool IsActivated() const;
    
    // إدارة الأخطاء
    std::string GetLastError() const;
    
private:
    LicenseManager();
    LicenseManager(const LicenseManager&) = delete;
    LicenseManager& operator=(const LicenseManager&) = delete;
    
    // ملفات الترخيص
    bool LoadLicense();
    bool SaveLicense();
    bool CreateTrialLicense();
    
    // التحقق من الصحة
    bool ValidateLicense();
    bool CheckExpiry();
    bool CheckHardware();
    
    // التشفير
    std::string EncryptLicenseData(const std::string& data);
    std::string DecryptLicenseData(const std::string& encrypted);
    
    // بيانات الترخيص
    struct LicenseData {
        std::string licenseKey;
        std::string hardwareId;
        std::string customerName;
        std::string activationDate;
        std::string expiryDate;
        std::string licenseType;  // "trial", "personal", "business"
        std::vector<uint8_t> signature;
    };
    
    LicenseData licenseData_;
    std::string lastError_;
    bool isInitialized_;
    bool isValid_;
    
    // مسارات الملفات
    std::string GetLicenseFilePath() const;
    std::string GetAppDataPath() const;
    
    // تواريخ
    static std::string GetCurrentDate();
    static std::string AddDaysToDate(const std::string& date, int days);
    static int DaysBetweenDates(const std::string& date1, const std::string& date2);
};

#endif // LICENSE_MANAGER_H