# windows/runner/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(runner LANGUAGES CXX)

# تعريف الملفات المصدرية للأمان
set(SECURITY_SOURCES
    "security/license_manager.cpp"
    "security/encryption_winapi.cpp"
    "security/hardware_id.cpp"
    "security/anti_tamper.cpp"
)

# التطبيق التنفيذي
add_executable(${BINARY_NAME} WIN32
    "flutter_window.cpp"
    "main.cpp"
    "utils.cpp"
    "win32_window.cpp"
    ${SECURITY_SOURCES}
    "${FLUTTER_MANAGED_DIR}/generated_plugin_registrant.cc"
    "Runner.rc"
    "runner.exe.manifest"
)

# تطبيق الإعدادات القياسية
apply_standard_settings(${BINARY_NAME})

# تعريفات إصدار Flutter
target_compile_definitions(${BINARY_NAME} PRIVATE "FLUTTER_VERSION=\"${FLUTTER_VERSION}\"")
target_compile_definitions(${BINARY_NAME} PRIVATE "FLUTTER_VERSION_MAJOR=${FLUTTER_VERSION_MAJOR}")
target_compile_definitions(${BINARY_NAME} PRIVATE "FLUTTER_VERSION_MINOR=${FLUTTER_VERSION_MINOR}")
target_compile_definitions(${BINARY_NAME} PRIVATE "FLUTTER_VERSION_PATCH=${FLUTTER_VERSION_PATCH}")
target_compile_definitions(${BINARY_NAME} PRIVATE "FLUTTER_VERSION_BUILD=${FLUTTER_VERSION_BUILD}")

# تعطيل تعريفات Windows المتضاربة
target_compile_definitions(${BINARY_NAME} PRIVATE "NOMINMAX")

# مكتبات الربط
target_link_libraries(${BINARY_NAME} PRIVATE flutter flutter_wrapper_app)
target_link_libraries(${BINARY_NAME} PRIVATE
    "dwmapi.lib"
    "advapi32.lib"      # للتسجيل والـ CryptoAPI
    "crypt32.lib"       # لوظائف التشفير
    "iphlpapi.lib"      # لـ MAC address
    "shell32.lib"       # لـ ShellExecute
    "shlwapi.lib"       # لوظائف Shell
)

# مجلدات العناوين
target_include_directories(${BINARY_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/security"
)

# تبعية Flutter
add_dependencies(${BINARY_NAME} flutter_assemble)