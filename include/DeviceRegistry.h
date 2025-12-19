#ifndef DEVICEREGISTRY_H
#define DEVICEREGISTRY_H

#ifdef UNIT_TEST
#ifdef TEST_WITH_FLASH
#define USE_FLASH true
#else
#define USE_FLASH false
#endif
#else
#define USE_FLASH true
#endif

#include <array>
#include <Preferences.h>

using MacArray = std::array<uint8_t, 6>;
constexpr uint8_t REGISTRY_ARRAY_SIZE = 251;
// First 251 IDs (0-250) used for devices, (251-255) reserved for error handling

enum class RegistryStatus : uint8_t
{
    SUCCESS = 251,
    ERROR_INVALID_ID = 252,
    ERROR_MAC_NOT_FOUND = 253
};

class DeviceRegistry
{

public:
    DeviceRegistry();

    const uint8_t *getMac(uint8_t id) const;
    const MacArray getRawMac(uint8_t id) const;

    const uint8_t getIdFromMac(const uint8_t *macPtr) const;
    const uint8_t getIdFromMac(MacArray &targetMac) const;

    uint8_t setMac(uint8_t id, const uint8_t *macPtr);
    uint8_t setMac(uint8_t id, const std::array<uint8_t, 6> &mac);

    void saveToFlash();
#ifdef UNIT_TEST
    void readFromFlash();
#endif

private:
    struct DeviceEntry
    {
        MacArray mac{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        bool active = false;
    };

    std::array<DeviceEntry, REGISTRY_ARRAY_SIZE> table{};

    Preferences prefs;

    static constexpr const char *REGISTRY_NAMESPACE = "dReg";
    static constexpr const char *REGISTRY_KEY = "val";

#ifndef UNIT_TEST
    void readFromFlash();
#endif
};

#endif