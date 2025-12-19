#include <DeviceRegistry.h>

DeviceRegistry::DeviceRegistry()
{
#if USE_FLASH
    DeviceRegistry::readFromFlash();
#endif
}

const uint8_t *DeviceRegistry::getMac(uint8_t id) const
{
    return table[id].mac.data();
}

const std::array<uint8_t, 6> DeviceRegistry::getRawMac(uint8_t id) const
{
    return table[id].mac;
}

const uint8_t DeviceRegistry::getIdFromMac(const uint8_t *macPtr) const
{
    for (uint8_t i = 0; i < sizeof(table); ++i)
    {
        if (memcmp(table[i].mac.data(), macPtr, 6) == 0)
            return i;
    }
    return 0xFF;
}

const uint8_t DeviceRegistry::getIdFromMac(std::array<uint8_t, 6> &targetMac) const
{
    for (uint8_t i = 0; i < sizeof(table); ++i)
    {
        if (table[i].mac == targetMac)
            return i;
    }
    return 0xFF;
}

void DeviceRegistry::setMac(uint8_t id, const uint8_t *macPtr)
{
    memcpy(table[id].mac.data(), macPtr, 6);
    table[id].active = true;
}

void DeviceRegistry::setMac(uint8_t id, const std::array<uint8_t, 6> &mac)
{
    table[id].mac = mac;
    table[id].active = true;
}

void DeviceRegistry::saveToFlash()
{
#if USE_FLASH
    prefs.begin(REGISTRY_NAMESPACE, false);
    prefs.putBytes(REGISTRY_KEY, (const uint8_t *)&table, sizeof(table));
    prefs.end();
#endif
}

void DeviceRegistry::readFromFlash()
{
#if USE_FLASH
    prefs.begin(REGISTRY_NAMESPACE, false);
    prefs.getBytes(REGISTRY_KEY, (uint8_t *)&table, sizeof(table));
    prefs.end();
#endif
}