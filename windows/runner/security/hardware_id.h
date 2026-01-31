// windows/runner/security/hardware_id.h
#ifndef HARDWARE_ID_H
#define HARDWARE_ID_H

#include <string>

class HardwareID {
public:
    static std::string GetUniqueID();
    static std::string GetMachineHash();
    
    // مكونات الهاردوير الفردية
    static std::string GetCPUID();
    static std::string GetDiskSerial();
    static std::string GetMACAddress();
    static std::string GetMotherboardInfo();
    static std::string GetWindowsID();
    
private:
    static std::string HashComponents();
};

#endif // HARDWARE_ID_H