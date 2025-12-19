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

class DeviceRegistry
{

public:
    DeviceRegistry();

    const uint8_t *getMac(uint8_t id) const;
    const MacArray getRawMac(uint8_t id) const;

    const uint8_t getIdFromMac(const uint8_t *macPtr) const;
    const uint8_t getIdFromMac(MacArray &targetMac) const;

    void setMac(uint8_t id, const uint8_t *macPtr);
    void setMac(uint8_t id, const std::array<uint8_t, 6> &mac);

    void saveToFlash();

private:
    struct DeviceEntry
    {
        MacArray mac{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        bool active = false;
    };

    std::array<DeviceEntry, sizeof(uint8_t)> table{};

    Preferences prefs;

    static constexpr const char *REGISTRY_NAMESPACE = "dReg";
    static constexpr const char *REGISTRY_KEY = "val";

    void readFromFlash();
};

#endif