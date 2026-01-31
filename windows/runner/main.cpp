// windows/runner/main.cpp
#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>
#include <shellapi.h>

#include "flutter_window.h"
#include "utils.h"
#include "security/license_manager.h"

// دالة لعرض رسالة خطأ
void ShowErrorMessage(const std::wstring& message, const std::wstring& title = L"خطأ") {
    MessageBoxW(NULL, message.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
}

// دالة لعرض رسالة معلومات
void ShowInfoMessage(const std::wstring& message, const std::wstring& title = L"معلومات") {
    MessageBoxW(NULL, message.c_str(), title.c_str(), MB_ICONINFORMATION | MB_OK);
}

// نافذة إدخال مفتاح الترخيص
bool ShowActivationDialog(std::string& licenseKey) {
    // نافذة إدخال بسيطة
    const int bufferSize = 256;
    wchar_t inputBuffer[bufferSize] = L"";
    
    // يمكن استخدام InputBox من مكتبة Windows
    // أو إنشاء نافذة مخصصة
    
    // هنا نستخدم MessageBox مع إدخال نصي بسيط
    // (في التطبيق الحقيقي، أنصح بإنشاء نافذة مخصصة)
    
    // للتبسيط، نستخدم MessageBox مع زرين
    int result = MessageBoxW(NULL,
        L"الترخيص غير مفعل أو منتهي الصلاحية.\n"
        L"هل تريد تفعيل التطبيق الآن؟",
        L"تفعيل التطبيق",
        MB_YESNO | MB_ICONQUESTION);
    
    if (result == IDYES) {
        // فتح موقع الويب أو عرض التعليمات
        ShellExecuteW(NULL, L"open", 
            L"https://your-website.com/activate", 
            NULL, NULL, SW_SHOWNORMAL);
        
        // طلب إدخال المفتاح
        MessageBoxW(NULL,
            L"بعد الحصول على مفتاح الترخيص،\n"
            L"الرجاء إدخاله في النافذة التالية.",
            L"تعليمات",
            MB_OK | MB_ICONINFORMATION);
        
        // هنا يمكنك إنشاء نافذة إدخال مخصصة
        // للتبسيط، نستخدم قيمة افتراضية
        licenseKey = "TRIAL-EXTENDED";
        return true;
    }
    
    return false;
}

// التحقق من الترخيص
bool CheckApplicationLicense() {
    LicenseManager& licenseMgr = LicenseManager::GetInstance();
    
    // محاولة التهيئة
    if (!licenseMgr.Initialize()) {
        ShowErrorMessage(L"فشل في تهيئة نظام الترخيص");
        return false;
    }
    
    // التحقق من الترخيص
    if (!licenseMgr.CheckLicense()) {
        std::string error = licenseMgr.GetLastError();
        
        // تحويل الخطأ إلى Unicode
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, error.c_str(), -1, NULL, 0);
        std::wstring wideError(wideLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, error.c_str(), -1, &wideError[0], wideLen);
        
        // عرض رسالة الخطأ
        std::wstring message = L"خطأ في الترخيص: " + wideError + L"\n\n";
        
        if (licenseMgr.IsTrial()) {
            int daysLeft = licenseMgr.GetRemainingDays();
            message += L"الأيام المتبقية في الفترة التجريبية: " + 
                      std::to_wstring(daysLeft) + L" يوم\n\n";
            
            if (daysLeft <= 7) {
                message += L"الفترة التجريبية ستنتهي قريباً.\n";
                message += L"الرجاء التفعيل للاستمرار.";
            }
        }
        
        // عرض الخيارات
        int response = MessageBoxW(NULL, message.c_str(),
            L"مشكلة في الترخيص",
            MB_YESNOCANCEL | MB_ICONWARNING);
        
        if (response == IDYES) {
            // محاولة التفعيل
            std::string licenseKey;
            if (ShowActivationDialog(licenseKey)) {
                if (licenseMgr.Activate(licenseKey)) {
                    ShowInfoMessage(L"تم تفعيل التطبيق بنجاح!");
                    return true; // إعادة المحاولة
                } else {
                    std::string activateError = licenseMgr.GetLastError();
                    ShowErrorMessage(L"فشل في التفعيل: " + 
                        std::wstring(activateError.begin(), activateError.end()));
                }
            }
        } else if (response == IDNO) {
            // الاستمرار في الوضع التجريبي
            if (licenseMgr.GetRemainingDays() > 0) {
                return true;
            }
        }
        
        return false;
    }
    
    // الترخيص صالح
    if (licenseMgr.IsTrial()) {
        int daysLeft = licenseMgr.GetRemainingDays();
        if (daysLeft <= 3) {
            std::wstring warning = L"تنبيه: الفترة التجريبية ستنتهي خلال " +
                                  std::to_wstring(daysLeft) + L" أيام.\n" +
                                  L"الرجاء التفعيل للاستمرار.";
            ShowInfoMessage(warning, L"تنبيه تجريبي");
        }
    }
    
    return true;
}

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
                      _In_ wchar_t *command_line, _In_ int show_command) {
    
    // التحقق من الترخيص أولاً
    if (!CheckApplicationLicense()) {
        return EXIT_FAILURE;
    }
    
    // إرفاق وحدة التحكم إذا كان التطبيق يعمل من سطر الأوامر
    if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent()) {
        CreateAndAttachConsole();
    }
    
    // تهيئة COM
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    // إعداد مشروع Flutter
    flutter::DartProject project(L"data");
    
    // معالجة وسائط سطر الأوامر
    std::vector<std::string> command_line_arguments = GetCommandLineArguments();
    project.set_dart_entrypoint_arguments(std::move(command_line_arguments));
    
    // إنشاء النافذة
    FlutterWindow window(project);
    Win32Window::Point origin(10, 10);
    Win32Window::Size size(1280, 720);
    
    if (!window.Create(L"Maher - النسخة المحمية", origin, size)) {
        return EXIT_FAILURE;
    }
    
    window.SetQuitOnClose(true);
    
    // حلقة الرسائل
    ::MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    
    // تنظيف
    ::CoUninitialize();
    return EXIT_SUCCESS;
}