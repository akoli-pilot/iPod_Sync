#include <napi.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

struct Device
{
    std::string id;
    std::string name;
    std::string model;      
    std::string modelCode;
    std::string serial;
    bool isMobile = false;
    uint64_t capacityBytes = 0;
    uint64_t freeBytes = 0;
};

static bool guessLegacyFromString(const std::string &raw, int &model, int &submodel);

// Legacy model strings mirror foo_dop/device_info.cpp
static const char * kModelStrings[] =
{ "1G", "2G", "3G", "4G", "4G Photo/Color", "5G", "5.5G", "Classic/6G (2007)", "Classic/6G (2008)", "Classic/6G (2009)", "Mini 1G", "Mini 2G", "Nano 1G", "Nano 2G", "Nano 3G", "Nano 4G", "Nano 5G", "Nano 6G", "Nano 7G", "Shuffle 1G", "Shuffle 2G", "Shuffle 3G", "Shuffle 4G" };

static const char * k5gModelStrings[] = { "White","Black","U2" };
static const char * k6gModelStrings[] = { "Black","Silver" };
static const char * k6_1gModelStrings[] = { "Black","Silver" };
static const char * k6_2gModelStrings[] = { "Black","Silver" };
static const char * kMini1gModelStrings[] = { "Silver","Green","Pink","Blue","Gold" };
static const char * kMini2gModelStrings[] = { "Silver","Green","Pink","Blue" };
static const char * k4gModelStrings[] = { "White","U2" };
static const char * kNano1gModelStrings[] = { "White","Black" };
static const char * kNano2gModelStrings[] = { "Black","Pink","Green","Blue","Silver","Red" };
static const char * kNano3gModelStrings[] = { "Pink","Black","Red","Green","Blue","Silver" };
static const char * kNano4gModelStrings[] = { "Black","Red","Yellow","Green","Orange","Purple","Pink","Blue","Silver" };
static const char * kNano5gModelStrings[] = { "Silver","Black","Purple","Blue","Green","Yellow","Orange","Red","Pink" };
static const char * kNano6gModelStrings[] = { "Silver","Graphite","Blue","Green","Orange","Pink","Red" };
static const char * kNano7gModelStrings[] = {
    "Pink (2012)","Yellow (2012)","Blue (2012)","Green (2012)","Purple (2012)","Silver (2012)","Slate (2012)","Red (2012)","Space Grey (2013)",
    "Pink (2015)","Gold (2015)","Blue (2015)","Silver (2015)","Space Grey (2015)","Red (2015)"
};
static const char * kShuffle2gModelStrings[] = {
    "Orange (2006)","Pink (2006)","Green (2006)","Blue (2006)","Silver (2006)",
    "Purple (2007)", "Green (2007)", "Blue (2007)", "Silver (2007)",
    "Purple (2008)", "Blue (2008)", "Green (2008)", "Red (2008)"
};
static const char * kShuffle3gModelStrings[] = { "Black","Silver","Green","Blue","Pink","Aluminium" };
static const char * kShuffle4gModelStrings[] = {
    "Silver (2010)","Pink (2010)","Orange (2010)","Green (2010)","Blue (2010)",
    "Pink (2012)","Green (2012)","Blue (2012)","Yellow (2012)","Silver (2012)","Purple (2012)","Slate (2012)","Red (2012)","Space Grey (2013)",
    "Pink (2015)","Gold (2015)","Blue (2015)","Silver (2015)","Space Grey (2015)","Red (2015)"
};

// label matching foo_dop g_get_model_string
static std::string legacyModelString(int model, int submodel)
{
    std::string out = "iPod ";
    auto addSub = [&](const char **arr, size_t sz) {
        if (submodel >= 0 && static_cast<size_t>(submodel) < sz) { out += " "; out += arr[submodel]; }
    };
    if (model >= 0 && static_cast<size_t>(model) < sizeof(kModelStrings)/sizeof(kModelStrings[0]))
        out += kModelStrings[model];

    switch (model)
    {
    case 5: // ipod_5g
    case 6: // ipod_5_5g
        addSub(k5gModelStrings, sizeof(k5gModelStrings)/sizeof(k5gModelStrings[0]));
        break;
    case 7: // ipod_6g
        addSub(k6gModelStrings, sizeof(k6gModelStrings)/sizeof(k6gModelStrings[0]));
        break;
    case 8: // ipod_6_1g
        addSub(k6_1gModelStrings, sizeof(k6_1gModelStrings)/sizeof(k6_1gModelStrings[0]));
        break;
    case 9: // ipod_6_2g
        addSub(k6_2gModelStrings, sizeof(k6_2gModelStrings)/sizeof(k6_2gModelStrings[0]));
        break;
    case 10: // ipod_mini_1g
        addSub(kMini1gModelStrings, sizeof(kMini1gModelStrings)/sizeof(kMini1gModelStrings[0]));
        break;
    case 11: // ipod_mini_2g
        addSub(kMini2gModelStrings, sizeof(kMini2gModelStrings)/sizeof(kMini2gModelStrings[0]));
        break;
    case 3: // ipod_4g
    case 4: // ipod_4g_colour
        addSub(k4gModelStrings, sizeof(k4gModelStrings)/sizeof(k4gModelStrings[0]));
        break;
    case 12: // ipod_nano_1g
        addSub(kNano1gModelStrings, sizeof(kNano1gModelStrings)/sizeof(kNano1gModelStrings[0]));
        break;
    case 13: // ipod_nano_2g
        addSub(kNano2gModelStrings, sizeof(kNano2gModelStrings)/sizeof(kNano2gModelStrings[0]));
        break;
    case 14: // ipod_nano_3g
        addSub(kNano3gModelStrings, sizeof(kNano3gModelStrings)/sizeof(kNano3gModelStrings[0]));
        break;
    case 15: // ipod_nano_4g
        addSub(kNano4gModelStrings, sizeof(kNano4gModelStrings)/sizeof(kNano4gModelStrings[0]));
        break;
    case 16: // ipod_nano_5g
        addSub(kNano5gModelStrings, sizeof(kNano5gModelStrings)/sizeof(kNano5gModelStrings[0]));
        break;
    case 17: // ipod_nano_6g
        addSub(kNano6gModelStrings, sizeof(kNano6gModelStrings)/sizeof(kNano6gModelStrings[0]));
        break;
    case 18: // ipod_nano_7g
        addSub(kNano7gModelStrings, sizeof(kNano7gModelStrings)/sizeof(kNano7gModelStrings[0]));
        break;
    case 20: // ipod_shuffle_2g
        addSub(kShuffle2gModelStrings, sizeof(kShuffle2gModelStrings)/sizeof(kShuffle2gModelStrings[0]));
        break;
    case 21: // ipod_shuffle_3g
        addSub(kShuffle3gModelStrings, sizeof(kShuffle3gModelStrings)/sizeof(kShuffle3gModelStrings[0]));
        break;
    case 22: // ipod_shuffle_4g
        addSub(kShuffle4gModelStrings, sizeof(kShuffle4gModelStrings)/sizeof(kShuffle4gModelStrings[0]));
        break;
    default:
        break;
    }
    return out;
}

static std::string describeProductType(const std::string &productType)
{
    static const std::unordered_map<std::string, std::string> kMap = {
        {"iPod1,1", "iPod touch (1st gen)"},
        {"iPod2,1", "iPod touch (2nd gen)"},
        {"iPod3,1", "iPod touch (3rd gen)"},
        {"iPod4,1", "iPod touch (4th gen)"},
        {"iPod5,1", "iPod touch (5th gen)"},
        {"iPod7,1", "iPod touch (6th gen)"},
        {"iPod9,1", "iPod touch (7th gen)"}
    };
    auto it = kMap.find(productType);
    if (it != kMap.end()) return it->second;
    return productType;
}

static bool containsInsensitive(const std::string &haystack, const std::string &needle)
{
    if (needle.empty()) return true;
    std::string hs = haystack, nd = needle;
    std::transform(hs.begin(), hs.end(), hs.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    std::transform(nd.begin(), nd.end(), nd.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return hs.find(nd) != std::string::npos;
}

static std::string findMountpoint(const std::string &devnode)
{
    std::ifstream f("/proc/mounts");
    if (!f.is_open()) return {};

    std::string devBase;
    {
        auto pos = devnode.find_last_of('/') ;
        devBase = (pos == std::string::npos) ? devnode : devnode.substr(pos + 1);
    }

    std::string line;
    while (std::getline(f, line))
    {
        std::istringstream iss(line);
        std::string dev, mount;
        if (!(iss >> dev >> mount)) continue;

        if (dev == devnode) return mount;

        // Resolve symlinked sources back to real device
        char resolved[PATH_MAX];
        if (realpath(dev.c_str(), resolved))
        {
            if (devnode == resolved) return mount;
            std::string resBase = resolved;
            auto pos2 = resBase.find_last_of('/') ;
            resBase = (pos2 == std::string::npos) ? resBase : resBase.substr(pos2 + 1);
            if (resBase == devBase) return mount;
        }
    }
    return {};
}

static uint64_t readCapacityBytes(const char *sectorsStr)
{
    if (!sectorsStr) return 0;
    char *end = nullptr;
    unsigned long long sectors = std::strtoull(sectorsStr, &end, 10);
    if (!end || end == sectorsStr) return 0;
    return sectors * 512ull; // Kernel reports 512-byte sectors for USB mass storage.
}

static std::string trim(const std::string &s)
{
    size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

struct SysInfoData
{
    std::string serial;
    std::string modelNumStr;
};

static SysInfoData readSysInfo(const std::string &mountpoint)
{
    SysInfoData info;
    if (mountpoint.empty()) return info;

    const char *candidates[] = {
        "/iPod_Control/Device/SysInfoExtended",
        "/iPod_Control/Device/SysInfo"
    };

    for (const char *rel : candidates)
    {
        std::string path = mountpoint + rel;
        std::ifstream f(path);
        if (!f.is_open()) continue;
        std::string line;
        while (std::getline(f, line))
        {
            auto colon = line.find(':');
            if (colon == std::string::npos) continue;
            std::string key = trim(line.substr(0, colon));
            std::string val = trim(line.substr(colon + 1));
            if (key == "pszSerialNumber" || key == "serialNumber") info.serial = val;
            else if (key == "ModelNumStr" || key == "modelNumber" || key == "ModelNum") info.modelNumStr = val;
        }
        if (!info.serial.empty() || !info.modelNumStr.empty()) break;
    }
    return info;
}

static void fillFreeSpace(Device &d, const std::string &mountpoint)
{
    if (mountpoint.empty()) return;
    struct statvfs vfs {};
    if (statvfs(mountpoint.c_str(), &vfs) == 0)
    {
        d.capacityBytes = static_cast<uint64_t>(vfs.f_blocks) * vfs.f_frsize;
        d.freeBytes = static_cast<uint64_t>(vfs.f_bfree) * vfs.f_frsize;
    }
}

static bool fileExists(const std::string &path)
{
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0;
}

struct MountEntry
{
    std::string device;
    std::string mountpoint;
};

// Fallback detection from libgpod's mount scanning on Linux when libudev is unavailable
static std::vector<MountEntry> enumerateMountedBlockDevices()
{
    std::vector<MountEntry> mounts;
    std::ifstream f("/proc/mounts");
    if (!f.is_open()) return mounts;

    std::string line;
    while (std::getline(f, line))
    {
        std::istringstream iss(line);
        std::string dev, mount, fs, opts;
        if (!(iss >> dev >> mount >> fs >> opts)) continue;

        // Only consider real block device entries.
        if (dev.rfind("/dev/", 0) != 0) continue;

        mounts.push_back({dev, mount});
    }
    return mounts;
}

static bool mountLooksLikeIpod(const std::string &mountpoint)
{
    const char *candidates[] = {
        "/iPod_Control/Device/SysInfoExtended",
        "/iPod_Control/Device/SysInfo"          // It was blank on my iPod 5.5g
    };
    for (const char *rel : candidates)
    {
        if (fileExists(mountpoint + rel)) return true;
    }
    return false;
}

static std::vector<Device> listMountedIpodVolumes()
{
    std::vector<Device> out;
    for (const auto &m : enumerateMountedBlockDevices())
    {
        if (!mountLooksLikeIpod(m.mountpoint)) continue;

        SysInfoData sysinfo = readSysInfo(m.mountpoint);

        Device d;
        d.isMobile = false;
        d.serial = sysinfo.serial;
        d.modelCode = sysinfo.modelNumStr;

        int legacyModel = -1, legacySub = -1;
        if (guessLegacyFromString(d.modelCode, legacyModel, legacySub))
            d.model = legacyModelString(legacyModel, legacySub);
        else if (!d.modelCode.empty())
            d.model = d.modelCode;
        else
            d.model = "iPod";

        std::string name = m.mountpoint;
        auto pos = name.find_last_of('/') ;
        if (pos != std::string::npos && pos + 1 < name.size())
            name = name.substr(pos + 1);
        d.name = name.empty() ? "iPod" : name;

        d.id = !d.serial.empty() ? d.serial : m.device;

        // Populate capacity
        fillFreeSpace(d, m.mountpoint);

        out.push_back(std::move(d));
    }
    return out;
}

static bool guessLegacyFromString(const std::string &raw, int &model, int &submodel)
{
    std::string s = raw;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    model = -1; submodel = -1;
    auto has = [&](const std::string &needle){ return s.find(needle) != std::string::npos; };

    if (has("classic")) { model = 7; return true; }
    if (has("nano 7") || has("nano7")) { model = 18; return true; }
    if (has("nano 6") || has("nano6")) { model = 17; return true; }
    if (has("nano 5") || has("nano5")) { model = 16; return true; }
    if (has("nano 4") || has("nano4")) { model = 15; return true; }
    if (has("nano 3") || has("nano3")) { model = 14; return true; }
    if (has("nano 2") || has("nano2")) { model = 13; return true; }
    if (has("nano 1") || has("nano1")) { model = 12; return true; }
    if (has("shuffle 4") || has("shuffle4")) { model = 22; return true; }
    if (has("shuffle 3") || has("shuffle3")) { model = 21; return true; }
    if (has("shuffle 2") || has("shuffle2")) { model = 20; return true; }
    if (has("shuffle")) { model = 19; return true; }
    if (has("mini 2") || has("mini2")) { model = 11; return true; }
    if (has("mini")) { model = 10; return true; }
    if (has("5.5") || has("5g")) { model = 6; return true; }
    if (has("4g") || has("color")) { model = 4; return true; }
    if (has("3g")) { model = 2; return true; }
    if (has("2g")) { model = 1; return true; }
    if (has("1g")) { model = 0; return true; }
    return false;
}

#ifdef HAVE_LIBIMOBILEDEVICE
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <plist/plist.h>
#endif

#ifdef HAVE_LIBUDEV
#include <libudev.h>
#endif

#ifdef HAVE_LIBIMOBILEDEVICE
static std::vector<Device> listMobileDevices()
{
    std::vector<Device> out;
    char **devices = nullptr;
    int count = 0;

    if (idevice_get_device_list(&devices, &count) != IDEVICE_E_SUCCESS || !devices) return out;

    for (int i = 0; i < count; i++)
    {
        const char *udid = devices[i];
        if (!udid) continue;

        idevice_t dev = nullptr;
        if (idevice_new(&dev, udid) != IDEVICE_E_SUCCESS) continue;

        lockdownd_client_t client = nullptr;
        if (lockdownd_client_new_with_handshake(dev, &client, "ipod-electron") != LOCKDOWN_E_SUCCESS)
        {
            idevice_free(dev);
            continue;
        }

        Device d;
        d.id = udid;
        d.isMobile = true;

        char *deviceName = nullptr;
        if (lockdownd_get_device_name(client, &deviceName) == LOCKDOWN_E_SUCCESS && deviceName)
        {
            d.name = deviceName;
            free(deviceName);
        }

        auto readValue = [&](const char *key) -> std::string
        {
            plist_t node = nullptr;
            std::string val;
            if (lockdownd_get_value(client, nullptr, key, &node) == LOCKDOWN_E_SUCCESS && node)
            {
                char *str = nullptr;
                plist_get_string_val(node, &str);
                if (str)
                {
                    val = str;
                    free(str);
                }
                plist_free(node);
            }
            return val;
        };

        d.serial = readValue("SerialNumber");
        d.modelCode = readValue("ProductType");
        d.model = describeProductType(d.modelCode);
        if (d.name.empty()) d.name = d.model.empty() ? "iPod" : d.model;

        lockdownd_client_free(client);
        idevice_free(dev);

        out.push_back(std::move(d));
    }

    idevice_device_list_free(devices);
    return out;
}
#else
static std::vector<Device> listMobileDevices()
{
    return {};
}
#endif

#ifdef HAVE_LIBUDEV
static std::vector<Device> listDiskDevices()
{
    std::vector<Device> out;
    struct udev *udev = udev_new();
    if (!udev) return out;

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);

    for (struct udev_list_entry *entry = devices; entry; entry = udev_list_entry_get_next(entry))
    {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);
        if (!dev) continue;

        // For partitions like /dev/sdb3, vendor/model often live on the parent disk node.
        struct udev_device *parent = udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");

        const char *model = udev_device_get_property_value(dev, "ID_MODEL");
        const char *vendor = udev_device_get_property_value(dev, "ID_VENDOR");
        const char *vendorId = udev_device_get_property_value(dev, "ID_VENDOR_ID");
        const char *modelId = udev_device_get_property_value(dev, "ID_MODEL_ID");
        const char *serial = udev_device_get_property_value(dev, "ID_SERIAL");
        const char *serialShort = udev_device_get_property_value(dev, "ID_SERIAL_SHORT");
        const char *fsLabel = udev_device_get_property_value(dev, "ID_FS_LABEL");
        const char *mediaPlayer = udev_device_get_property_value(dev, "ID_MEDIA_PLAYER");
        const char *sectorsStr = udev_device_get_sysattr_value(dev, "size");

        // Parent fallbacks
        if (!model && parent) model = udev_device_get_property_value(parent, "ID_MODEL");
        if (!vendor && parent) vendor = udev_device_get_property_value(parent, "ID_VENDOR");
        if (!vendorId && parent) vendorId = udev_device_get_property_value(parent, "ID_VENDOR_ID");
        if (!modelId && parent) modelId = udev_device_get_property_value(parent, "ID_MODEL_ID");
        if (!serial && parent) serial = udev_device_get_property_value(parent, "ID_SERIAL");
        if (!serialShort && parent) serialShort = udev_device_get_property_value(parent, "ID_SERIAL_SHORT");
        if (!fsLabel && parent) fsLabel = udev_device_get_property_value(parent, "ID_FS_LABEL");
        if (!mediaPlayer && parent) mediaPlayer = udev_device_get_property_value(parent, "ID_MEDIA_PLAYER");
        if (!sectorsStr && parent) sectorsStr = udev_device_get_sysattr_value(parent, "size");

        const char *devnode = udev_device_get_devnode(dev);

        std::string modelStr = model ? model : "";
        std::string vendorStr = vendor ? vendor : "";
        std::string vidStr = vendorId ? vendorId : "";
        std::string pidStr = modelId ? modelId : "";

        bool matchesVendor = containsInsensitive(vendorStr, "apple") || vidStr == "05ac";
        bool matchesModel = containsInsensitive(modelStr, "ipod") || containsInsensitive(mediaPlayer ? mediaPlayer : "", "apple_ipod");

        if (!matchesVendor && !matchesModel)
        {
            udev_device_unref(dev);
            continue;
        }

        Device d;
        d.isMobile = false;
        std::string mount = devnode ? findMountpoint(devnode) : std::string();
        SysInfoData sysinfo = readSysInfo(mount);

        d.modelCode = !modelStr.empty() ? modelStr : sysinfo.modelNumStr;
        int legacyModel = -1, legacySub = -1;
        if (guessLegacyFromString(d.modelCode, legacyModel, legacySub))
            d.model = legacyModelString(legacyModel, legacySub);
        else if (!sysinfo.modelNumStr.empty())
            d.model = sysinfo.modelNumStr;
        else
            d.model = d.modelCode.empty() ? "iPod" : d.modelCode;
        d.serial = serial ? serial : (serialShort ? serialShort : sysinfo.serial);
        d.id = !d.serial.empty() ? d.serial : (devnode ? devnode : "disk-ipod");
        d.name = fsLabel && *fsLabel ? fsLabel : (devnode ? devnode : "iPod");

        d.capacityBytes = readCapacityBytes(sectorsStr);

        if (!mount.empty())
        {
            fillFreeSpace(d, mount);
        }

        out.push_back(std::move(d));
        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return out;
}
#else
static std::vector<Device> listDiskDevices()
{
    return {};
}
#endif

static Napi::Value ListDevicesWrapped(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    std::vector<Device> devices;
    std::unordered_set<std::string> seen;

    auto append = [&](const std::vector<Device> &src)
    {
        for (const auto &d : src)
        {
            std::string key = !d.serial.empty() ? d.serial : (!d.id.empty() ? d.id : d.name);
            if (!key.empty() && !seen.insert(key).second) continue;
            devices.push_back(d);
        }
    };

    append(listMobileDevices());
    append(listDiskDevices());
    append(listMountedIpodVolumes());

    Napi::Array arr = Napi::Array::New(env, devices.size());
    for (size_t i = 0; i < devices.size(); i++)
    {
        const Device &d = devices[i];
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("id", Napi::String::New(env, d.id));
        obj.Set("name", Napi::String::New(env, d.name));
        obj.Set("model", Napi::String::New(env, d.model));
        obj.Set("modelCode", Napi::String::New(env, d.modelCode));
        obj.Set("serial", Napi::String::New(env, d.serial));
        obj.Set("isMobile", Napi::Boolean::New(env, d.isMobile));
        obj.Set("capacityBytes", Napi::Number::New(env, static_cast<double>(d.capacityBytes)));
        obj.Set("freeBytes", Napi::Number::New(env, static_cast<double>(d.freeBytes)));
        arr.Set(i, obj);
    }
    return arr;
}

static Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "listDevices"), Napi::Function::New(env, ListDevicesWrapped));
    return exports;
}

NODE_API_MODULE(ipod_native, Init)
