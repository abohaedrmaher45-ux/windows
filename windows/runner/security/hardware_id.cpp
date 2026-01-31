#include "hardware_id.h"
#include "encryption.h"
#include <windows.h>
#include <iphlpapi.h>
#include <intrin.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "iphlpapi.lib")

std::string HardwareID::GetUniqueID() {
    return GenerateHardwareHash();
}

std::string HardwareID::GenerateHardwareHash() {
    std::string components;
    
    components += GetCPUID();
    components += GetDiskSerial();
    components += GetMACAddress();
    components += GetMotherboardID();
    
    return Encryption::SHA256Hash(components).substr(0, 32);
}

std::string HardwareID::GetCPUID() {
    int cpuInfo[4] = { -1 };
    char cpuID[49];
    
    __cpuid(cpuInfo, 1);
    sprintf_s(cpuID, sizeof(cpuID),
        "%08X%08X%08X%08X",
        cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
    
    return std::string(cpuID);
}

std::string HardwareID::GetDiskSerial() {
    HANDLE hDevice = CreateFileW(
        L"\\\\.\\PhysicalDrive0",
        0, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        return "DISK_ERROR";
    }
    
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    
    STORAGE_DESCRIPTOR_HEADER header = {};
    DWORD bytesReturned = 0;
    
    DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                   &query, sizeof(query),
                   &header, sizeof(header),
                   &bytesReturned, NULL);
    
    std::vector<uint8_t> buffer(header.Size);
    
    DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                   &query, sizeof(query),
                   buffer.data(), buffer.size(),
                   &bytesReturned, NULL);
    
    CloseHandle(hDevice);
    
    STORAGE_DEVICE_DESCRIPTOR* desc = 
        reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer.data());
    
    if (desc->SerialNumberOffset) {
        return reinterpret_cast<char*>(buffer.data() + desc->SerialNumberOffset);
    }
    
    return "NO_SERIAL";
}

std::string HardwareID::GetMACAddress() {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwBufLen = sizeof(adapterInfo);
    
    if (GetAdaptersInfo(adapterInfo, &dwBufLen) != ERROR_SUCCESS) {
        return "MAC_ERROR";
    }
    
    PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
    if (pAdapterInfo) {
        std::ostringstream oss;
        for (UINT i = 0; i < pAdapterInfo->AddressLength; i++) {
            if (i == (pAdapterInfo->AddressLength - 1)) {
                oss << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(pAdapterInfo->Address[i]);
            } else {
                oss << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(pAdapterInfo->Address[i]) << "-";
            }
        }
        return oss.str();
    }
    
    return "NO_MAC";
}

std::string HardwareID::GetMotherboardID() {
    HKEY hKey;
    LPCSTR keyPath = "HARDWARE\\DESCRIPTION\\System\\BIOS";
    LPCSTR valueName = "BaseBoardProduct";
    
    char value[256];
    DWORD valueSize = sizeof(value);
    
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, valueName, NULL, NULL, 
                            reinterpret_cast<LPBYTE>(value), &valueSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(value);
        }
        RegCloseKey(hKey);
    }
    
    return "BOARD_UNKNOWN";
}