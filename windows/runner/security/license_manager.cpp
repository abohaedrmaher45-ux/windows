// windows/runner/security/license_manager.cpp
#include "license_manager.h"
#include "encryption_winapi.h"
#include "hardware_id.h"
#include "anti_tamper.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

// ==================== Singleton ====================
LicenseManager& LicenseManager::GetInstance() {
    static LicenseManager instance;
    return instance;
}

LicenseManager::LicenseManager() 
    : isInitialized_(false)
    , isValid_(false) {
}

// ==================== التهيئة ====================
bool LicenseManager::Initialize() {
    if (isInitialized_) {
        return isValid_;
    }
    
    // 1. فحص العبث أولاً
    if (AntiTamper::CheckForTampering()) {
        lastError_ = "تم اكتشاف محاولة العبث بالتطبيق";
        AntiTamper::TriggerDefense();
        return false;
    }
    
    // 2. إنشاء مجلد البيانات إذا لم يكن موجوداً
    std::string appDataPath = GetAppDataPath();
    CreateDirectoryA(appDataPath.c_str(), NULL);
    
    // 3. محاولة تحميل الترخيص
    if (!LoadLicense()) {
        // إذا فشل التحميل، ننشئ ترخيصاً تجريبياً
        if (!CreateTrialLicense()) {
            lastError_ = "فشل في إنشاء ترخيص تجريبي";
            return false;
        }
        
        if (!SaveLicense()) {
            lastError_ = "فشل في حفظ الترخيص التجريبي";
            return false;
        }
    }
    
    // 4. التحقق من صحة الترخيص
    if (!ValidateLicense()) {
        lastError_ = "الترخيص غير صالح";
        isValid_ = false;
    } else {
        isValid_ = true;
    }
    
    isInitialized_ = true;
    return isValid_;
}

// ==================== تحميل وحفظ الترخيص ====================
bool LicenseManager::LoadLicense() {
    std::string licensePath = GetLicenseFilePath();
    std::ifstream file(licensePath, std::ios::binary);
    
    if (!file.is_open()) {
        lastError_ = "ملف الترخيص غير موجود";
        return false;
    }
    
    // قراءة البيانات المشفرة
    std::string encryptedData(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    
    file.close();
    
    if (encryptedData.empty()) {
        lastError_ = "ملف الترخيص فارغ";
        return false;
    }
    
    // فك التشفير
    std::string licenseData = DecryptLicenseData(encryptedData);
    if (licenseData.empty()) {
        lastError_ = "فشل في فك تشفير ملف الترخيص";
        return false;
    }
    
    // تحليل البيانات
    std::istringstream iss(licenseData);
    std::string line;
    
    while (std::getline(iss, line)) {
        size_t separator = line.find('=');
        if (separator != std::string::npos) {
            std::string key = line.substr(0, separator);
            std::string value = line.substr(separator + 1);
            
            if (key == "LICENSE_KEY") licenseData_.licenseKey = value;
            else if (key == "HARDWARE_ID") licenseData_.hardwareId = value;
            else if (key == "CUSTOMER_NAME") licenseData_.customerName = value;
            else if (key == "ACTIVATION_DATE") licenseData_.activationDate = value;
            else if (key == "EXPIRY_DATE") licenseData_.expiryDate = value;
            else if (key == "LICENSE_TYPE") licenseData_.licenseType = value;
        }
    }
    
    return true;
}

bool LicenseManager::SaveLicense() {
    // تجميع بيانات الترخيص
    std::ostringstream oss;
    oss << "LICENSE_KEY=" << licenseData_.licenseKey << "\n"
        << "HARDWARE_ID=" << licenseData_.hardwareId << "\n"
        << "CUSTOMER_NAME=" << licenseData_.customerName << "\n"
        << "ACTIVATION_DATE=" << licenseData_.activationDate << "\n"
        << "EXPIRY_DATE=" << licenseData_.expiryDate << "\n"
        << "LICENSE_TYPE=" << licenseData_.licenseType;
    
    std::string licenseData = oss.str();
    
    // تشفير البيانات
    std::string encryptedData = EncryptLicenseData(licenseData);
    if (encryptedData.empty()) {
        lastError_ = "فشل في تشفير بيانات الترخيص";
        return false;
    }
    
    // حفظ الملف
    std::string licensePath = GetLicenseFilePath();
    std::ofstream file(licensePath, std::ios::binary);
    
    if (!file.is_open()) {
        lastError_ = "فشل في فتح ملف الترخيص للكتابة";
        return false;
    }
    
    file.write(encryptedData.c_str(), encryptedData.size());
    file.close();
    
    // إخفاء الملف
    SetFileAttributesA(licensePath.c_str(), FILE_ATTRIBUTE_HIDDEN);
    
    return true;
}

// ==================== الترخيص التجريبي ====================
bool LicenseManager::CreateTrialLicense() {
    licenseData_.licenseKey = "TRIAL-" + WinCrypt::GenerateRandomString(10);
    licenseData_.hardwareId = HardwareID::GetUniqueID();
    licenseData_.customerName = "Trial User";
    licenseData_.activationDate = GetCurrentDate();
    licenseData_.expiryDate = AddDaysToDate(licenseData_.activationDate, 30); // 30 يوم
    licenseData_.licenseType = "trial";
    
    return true;
}

// ==================== التحقق من الصحة ====================
bool LicenseManager::CheckLicense() {
    if (!isInitialized_ && !Initialize()) {
        return false;
    }
    
    // التحقق الدوري من العبث
    AntiTamper::TriggerDefense();
    
    return isValid_ && ValidateLicense();
}

bool LicenseManager::ValidateLicense() {
    // 1. التحقق من انتهاء الصلاحية
    if (!CheckExpiry()) {
        lastError_ = "انتهت صلاحية الترخيص";
        return false;
    }
    
    // 2. التحقق من مطابقة الهاردوير
    if (!CheckHardware()) {
        lastError_ = "الترخيص غير متوافق مع هذا الجهاز";
        return false;
    }
    
    // 3. التحقق من صحة المفتاح (يمكن إضافة المزيد)
    if (licenseData_.licenseKey.empty()) {
        lastError_ = "مفتاح الترخيص فارغ";
        return false;
    }
    
    return true;
}

bool LicenseManager::CheckExpiry() {
    if (licenseData_.expiryDate.empty()) {
        return false;
    }
    
    std::string currentDate = GetCurrentDate();
    int daysRemaining = DaysBetweenDates(currentDate, licenseData_.expiryDate);
    
    return daysRemaining >= 0;
}

bool LicenseManager::CheckHardware() {
    std::string currentHardwareId = HardwareID::GetUniqueID();
    return currentHardwareId == licenseData_.hardwareId;
}

// ==================== التفعيل ====================
bool LicenseManager::Activate(const std::string& licenseKey) {
    // هنا يمكنك إضافة منطق الاتصال بالسيرفر للتحقق
    // أو استخدام خوارزمية محلية
    
    if (licenseKey.length() < 10) {
        lastError_ = "مفتاح الترخيص قصير جداً";
        return false;
    }
    
    // تنفيذ بسيط للتحقق (يمكنك تعديله)
    bool isValidFormat = false;
    
    // تنسيق المفتاح: MAHER-XXXX-XXXX-XXXX
    if (licenseKey.substr(0, 6) == "MAHER-") {
        isValidFormat = true;
        licenseData_.licenseType = "premium";
    }
    // أو تنسيق تجاري
    else if (licenseKey.substr(0, 4) == "BUS-") {
        isValidFormat = true;
        licenseData_.licenseType = "business";
    }
    
    if (!isValidFormat) {
        lastError_ = "تنسيق مفتاح الترخيص غير صحيح";
        return false;
    }
    
    // تحديث بيانات الترخيص
    licenseData_.licenseKey = licenseKey;
    licenseData_.customerName = "Activated User";
    licenseData_.activationDate = GetCurrentDate();
    licenseData_.expiryDate = AddDaysToDate(licenseData_.activationDate, 365); // سنة كاملة
    licenseData_.hardwareId = HardwareID::GetUniqueID();
    
    // حفظ الترخيص الجديد
    if (!SaveLicense()) {
        lastError_ = "فشل في حفظ الترخيص المفعل";
        return false;
    }
    
    // إعادة التهيئة
    isValid_ = true;
    
    return true;
}

// ==================== التشفير ====================
std::string LicenseManager::EncryptLicenseData(const std::string& data) {
    std::vector<uint8_t> dataVec(data.begin(), data.end());
    
    // استخدام مفتاح مشتق من الهاردوير ID
    std::string encryptionKey = HardwareID::GetUniqueID() + "_MaherApp_2024";
    
    std::vector<uint8_t> encrypted = WinCrypt::AESEncrypt(dataVec, encryptionKey);
    
    if (encrypted.empty()) {
        // إذا فشل AES، نستخدم تشفير بسيط
        return WinCrypt::SimpleEncrypt(data, encryptionKey);
    }
    
    return std::string(encrypted.begin(), encrypted.end());
}

std::string LicenseManager::DecryptLicenseData(const std::string& encrypted) {
    std::string encryptionKey = HardwareID::GetUniqueID() + "_MaherApp_2024";
    
    // محاولة فك التشفير باستخدام AES أولاً
    std::vector<uint8_t> encryptedVec(encrypted.begin(), encrypted.end());
    std::vector<uint8_t> decrypted = WinCrypt::AESDecrypt(encryptedVec, encryptionKey);
    
    if (!decrypted.empty()) {
        return std::string(decrypted.begin(), decrypted.end());
    }
    
    // إذا فشل، نجرب التشفير البسيط
    return WinCrypt::SimpleDecrypt(encrypted, encryptionKey);
}

// ==================== مسارات الملفات ====================
std::string LicenseManager::GetAppDataPath() const {
    char appDataPath[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
    
    std::string path = std::string(appDataPath) + "\\MaherApp";
    return path;
}

std::string LicenseManager::GetLicenseFilePath() const {
    return GetAppDataPath() + "\\license.dat";
}

// ==================== معلومات الترخيص ====================
std::string LicenseManager::GetLicenseType() const {
    return licenseData_.licenseType;
}

int LicenseManager::GetRemainingDays() const {
    if (licenseData_.expiryDate.empty()) {
        return 0;
    }
    
    std::string currentDate = GetCurrentDate();
    return DaysBetweenDates(currentDate, licenseData_.expiryDate);
}

std::string LicenseManager::GetExpiryDate() const {
    return licenseData_.expiryDate;
}

bool LicenseManager::IsTrial() const {
    return licenseData_.licenseType == "trial";
}

bool LicenseManager::IsActivated() const {
    return licenseData_.licenseType != "trial";
}

std::string LicenseManager::GetLastError() const {
    return lastError_;
}

// ==================== دوال التاريخ ====================
std::string LicenseManager::GetCurrentDate() {
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);
    
    char buffer[11];
    sprintf_s(buffer, "%04d-%02d-%02d", 
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    
    return std::string(buffer);
}

std::string LicenseManager::AddDaysToDate(const std::string& date, int days) {
    std::tm tm = {};
    sscanf_s(date.c_str(), "%04d-%02d-%02d", 
             &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    
    std::time_t t = std::mktime(&tm);
    t += days * 24 * 60 * 60;
    
    localtime_s(&tm, &t);
    
    char buffer[11];
    sprintf_s(buffer, "%04d-%02d-%02d", 
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    
    return std::string(buffer);
}

int LicenseManager::DaysBetweenDates(const std::string& date1, const std::string& date2) {
    std::tm tm1 = {}, tm2 = {};
    
    sscanf_s(date1.c_str(), "%04d-%02d-%02d", 
             &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday);
    sscanf_s(date2.c_str(), "%04d-%02d-%02d", 
             &tm2.tm_year, &tm2.tm_mon, &tm2.tm_mday);
    
    tm1.tm_year -= 1900;
    tm1.tm_mon -= 1;
    tm2.tm_year -= 1900;
    tm2.tm_mon -= 1;
    
    std::time_t t1 = std::mktime(&tm1);
    std::time_t t2 = std::mktime(&tm2);
    
    double diff = std::difftime(t2, t1);
    return static_cast<int>(diff / (60 * 60 * 24));
}