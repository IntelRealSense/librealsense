#include "metadata-helper.h"

#ifdef WIN32

#include <Windows.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>

#include <regex>
#include <iostream>
#include <sstream>
#include <functional>
#include <map>
#include <memory>

#include <librealsense2/rs.hpp>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

#endif

namespace rs2
{
#ifdef WIN32
    struct device_id
    {
        std::string pid, mi, guid, sn;
    };

    inline std::string to_lower(std::string x)
    {
        transform(x.begin(), x.end(), x.begin(), tolower);
        return x;
    }

    bool operator==(const device_id& a, const device_id& b)
    {
        return (to_lower(a.pid) == to_lower(b.pid) &&
            to_lower(a.mi) == to_lower(b.mi) &&
            to_lower(a.guid) == to_lower(b.guid) &&
            to_lower(a.sn) == to_lower(b.sn));
    }

    class windows_metadata_helper : public metadata_helper
    {
    public:
        static bool parse_device_id(std::string id, device_id* res)
        {
            static const std::regex regex("[\\s\\S]*pid_([0-9a-fA-F]+)&mi_([0-9]+)#[0-9]&([0-9a-fA-F]+)&[\\s\\S]*\\{([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})\\}[\\s\\S]*");

            id = to_lower(id); // ignore case

            std::match_results<std::string::const_iterator> match;

            if (std::regex_match(id, match, regex) && match.size() > 4)
            {
                res->pid = match[1];
                res->mi = match[2];
                res->sn = match[3];
                res->guid = match[4];
                return true;
            }
            return false;
        }

        static void do_foreach_device(std::vector<device_id> devices, 
            std::function<void(device_id&, std::wstring)> action)
        {
            std::map<std::string, std::vector<device_id>> guid_to_devices;
            for (auto&& d : devices)
            {
                guid_to_devices[d.guid].push_back(d);
            }

            for (auto&& kvp : guid_to_devices)
            {
                auto guid = kvp.first;

                std::stringstream ss;
                ss << "SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{" << guid << "}";
                auto s = ss.str();
                std::wstring ws(s.begin(), s.end());

                HKEY hKey;
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, ws.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
                {
                    // Don't forget to release in the end:
                    std::shared_ptr<void> raii(hKey, RegCloseKey);

                    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
                    DWORD    cchClassName = MAX_PATH;  // size of class string 
                    DWORD    cSubKeys = 0;               // number of subkeys 
                    DWORD    cbMaxSubKey;              // longest subkey size 
                    DWORD    cchMaxClass;              // longest class string 
                    DWORD    cValues;              // number of values for key 
                    DWORD    cchMaxValue;          // longest value name 
                    DWORD    cbMaxValueData;       // longest value data 
                    DWORD    cbSecurityDescriptor; // size of security descriptor 
                    FILETIME ftLastWriteTime;      // last write time 

                    DWORD retCode = RegQueryInfoKey(
                        hKey,                    // key handle 
                        achClass,                // buffer for class name 
                        &cchClassName,           // size of class string 
                        NULL,                    // reserved 
                        &cSubKeys,               // number of subkeys 
                        &cbMaxSubKey,            // longest subkey size 
                        &cchMaxClass,            // longest class string 
                        &cValues,                // number of values for this key 
                        &cchMaxValue,            // longest value name 
                        &cbMaxValueData,         // longest value data 
                        &cbSecurityDescriptor,   // security descriptor 
                        &ftLastWriteTime);       // last write time 

                    for (int i = 0; i<cSubKeys; i++)
                    {
                        TCHAR achKey[MAX_KEY_LENGTH];
                        DWORD cbName = MAX_KEY_LENGTH;
                        retCode = RegEnumKeyEx(hKey, i,
                            achKey,
                            &cbName,
                            NULL,
                            NULL,
                            NULL,
                            &ftLastWriteTime);
                        if (retCode == ERROR_SUCCESS)
                        {
                            std::wstring ke = achKey;
                            device_id rdid;
                            if (parse_device_id(std::string(ke.begin(), ke.end()), &rdid))
                            {
                                for (auto&& did : kvp.second)
                                {
                                    if (rdid == did)
                                    {
                                        std::wstringstream ss;
                                        ss << ws << "\\" << ke << "\\#GLOBAL\\Device Parameters";
                                        auto s = ss.str();
                                        std::wstring ws(s.begin(), s.end());

                                        action(rdid, ws);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Heuristic that determines how many media-pins UVC device is expected to have
        static int number_of_mediapins(std::string pid, std::string mi)
        {
            if (mi == "00")
            {
                // L500 has 3 media-pins
                if (to_lower(pid) == "0b0d" || to_lower(pid) == "0b3d") return 3;
                else return 2; // D400 has two
            }
            return 1; // RGB has one
        }

        bool is_running_as_admin()
        {
            BOOL result = FALSE;
            PSID admin_group = NULL;

            SID_IDENTIFIER_AUTHORITY ntauthority = SECURITY_NT_AUTHORITY;
            if (!AllocateAndInitializeSid(
                &ntauthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &admin_group))
            {
                return false;
            }

            if (!CheckTokenMembership(NULL, admin_group, &result))
            {
                FreeSid(admin_group);
                return false;
            }

            return result;
        }

        bool elevate_to_admin()
        {
            if (!is_running_as_admin())
            {
                wchar_t szPath[MAX_PATH];
                if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
                {
                    SHELLEXECUTEINFO sei = { sizeof(sei) };

                    sei.lpVerb = L"runas";
                    sei.lpFile = szPath;
                    sei.hwnd = NULL;
                    sei.nShow = SW_NORMAL;
                    auto cmd_line = get_command_line_param();
                    std::wstring wcmd(cmd_line.begin(), cmd_line.end());
                    sei.lpParameters = wcmd.c_str();

                    ShellExecuteEx(&sei); // not checking return code - what you are going to do? retry? why?
                }

                return false;
            }
            else
            {
                return true;
            }
        }

        bool is_enabled(std::string id) const override
        {
            bool res = false;

            device_id did;
            if (parse_device_id(id, &did))
                do_foreach_device({ did }, [&res, did](device_id&, std::wstring path) {

                HKEY key;
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS)
                {
                    // Don't forget to release in the end:
                    std::shared_ptr<void> raii(key, RegCloseKey);

                    bool found = true;
                    DWORD len = sizeof(DWORD);//size of data

                    for (int i = 0; i < number_of_mediapins(did.pid, did.mi); i++)
                    {
                        std::wstringstream ss; ss << L"MetadataBufferSizeInKB" << i;
                        std::wstring metadatakey = ss.str();

                        DWORD MetadataBufferSizeInKB = 0;
                        RegQueryValueEx(
                            key,
                            metadatakey.c_str(),
                            NULL,
                            NULL,
                            (LPBYTE)(&MetadataBufferSizeInKB),
                            &len);

                        found = found && MetadataBufferSizeInKB;
                    }

                    if (found) res = true;
                }
            });

            return res;
        }

        void enable_metadata() override
        {
            if (elevate_to_admin()) // Elevation to admin was succesful?
            {
                std::vector<device_id> dids;

                rs2::context ctx;
                auto list = ctx.query_devices();
                for (int i = 0; i < list.size(); i++)
                {
                    try
                    {
                        rs2::device dev = list[i];
                        if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) && dev.supports(RS2_CAMERA_INFO_PHYSICAL_PORT))
                        {
                            std::string product = dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE);
                            if (product == "D400" || product == "SR300" || product == "L500")
                            {
                                std::string port = dev.get_info(RS2_CAMERA_INFO_PHYSICAL_PORT);
                                device_id did;
                                if (parse_device_id(port, &did)) dids.push_back(did);
                            }
                        }
                    }
                    catch (...) {}
                }

                do_foreach_device(dids, [](device_id& did, std::wstring path) {
                    HKEY key;
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_WRITE | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS)
                    {
                        // Don't forget to release in the end:
                        std::shared_ptr<void> raii(key, RegCloseKey);

                        bool found = true;
                        DWORD len = sizeof(DWORD);//size of data

                        for (int i = 0; i < number_of_mediapins(did.pid, did.mi); i++)
                        {
                            std::wstringstream ss; ss << L"MetadataBufferSizeInKB" << i;
                            std::wstring metadatakey = ss.str();

                            DWORD MetadataBufferSizeInKB = 5;
                            RegSetValueEx(key, metadatakey.c_str(), 0, REG_DWORD,
                                (const BYTE*)&MetadataBufferSizeInKB, sizeof(DWORD));
                            // What to do if failed???
                        }
                    }
                });
            }
        }
    };
#endif

    metadata_helper& metadata_helper::instance() 
    {
#ifdef WIN32
        static windows_metadata_helper instance;
#else
        static metadata_helper instance;
#endif
        return instance;
    }
}