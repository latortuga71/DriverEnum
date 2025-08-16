from ctypes import *
import wmi    # Windows Management Instrumentation .
import win32file
import win32con

INVALID_HANDLE_VALUE = -1

def get_driver_info():
    c = wmi.WMI()
    drivers = c.Win32_SystemDriver()
    driver_list = []

    for driver in drivers:
        info = {
            "Name": getattr(driver, "Name", "N/A"),
            "DisplayName": getattr(driver, "DisplayName", "N/A"),
            "PathName": getattr(driver, "PathName", "N/A"),
            "State": getattr(driver, "State", "N/A"),
            "StartMode": getattr(driver, "StartMode", "N/A"),
            "ServiceType": getattr(driver, "ServiceType", "N/A"),
            "Description": getattr(driver, "Description", "N/A"),
            "ErrorControl": getattr(driver, "ErrorControl", "N/A"),
            "LoadOrderGroup": getattr(driver, "LoadOrderGroup", "N/A"),
            "Dependencies": ', '.join(getattr(driver, "Dependencies", [])) or "None"
        }
        driver_list.append(info)
    
    return driver_list

for d in get_driver_info():
    print(d["Name"])
