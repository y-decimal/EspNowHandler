#include <DeviceRegistry.h>

DeviceRegistry::DeviceRegistry()
{
#if USE_FLASH
    DeviceRegistry::readFromFlash();
#endif
}

const uint8_t *DeviceRegistry::getMac(uint8_t id) const
{
    if (id >= REGISTRY_ARRAY_SIZE)
        return nullptr;
    return table[id].mac.data();
}

const std::array<uint8_t, 6> DeviceRegistry::getRawMac(uint8_t id) const
{
    if (id >= REGISTRY_ARRAY_SIZE)
        return MacArray{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return table[id].mac;
}

const uint8_t DeviceRegistry::getIdFromMac(const uint8_t *macPtr) const
{
    for (uint8_t i = 0; i <= REGISTRY_ARRAY_SIZE; ++i)
    {
        if (memcmp(table[i].mac.data(), macPtr, 6) == 0 && table[i].active)
            return i;
    }
    return (uint8_t)RegistryStatus::ERROR_MAC_NOT_FOUND;
}

const uint8_t DeviceRegistry::getIdFromMac(std::array<uint8_t, 6> &targetMac) const
{
    for (uint8_t i = 0; i <= REGISTRY_ARRAY_SIZE; ++i)
    {
        if (table[i].mac == targetMac && table[i].active)
            return i;
    }
    return (uint8_t)RegistryStatus::ERROR_MAC_NOT_FOUND;
}

uint8_t DeviceRegistry::setMac(uint8_t id, const uint8_t *macPtr)
{
    if (id >= REGISTRY_ARRAY_SIZE)
        return (uint8_t)RegistryStatus::ERROR_INVALID_ID;
    memcpy(table[id].mac.data(), macPtr, 6);
    table[id].active = true;
    return (uint8_t)RegistryStatus::SUCCESS;
}

uint8_t DeviceRegistry::setMac(uint8_t id, const std::array<uint8_t, 6> &mac)
{
    if (id >= REGISTRY_ARRAY_SIZE)
        return (uint8_t)RegistryStatus::ERROR_INVALID_ID;
    table[id].mac = mac;
    table[id].active = true;
    return (uint8_t)RegistryStatus::SUCCESS;
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
    if (prefs.getBytes(REGISTRY_KEY, (uint8_t *)&table, sizeof(table)) == 0)
    {
        // If no data was read, initialize the table to default values
        table.fill(DeviceEntry{});
    }
    prefs.end();
#endif
}