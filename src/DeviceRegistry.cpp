#include <DeviceRegistry.h>

DeviceRegistry::DeviceRegistry()
{
#if USE_FLASH
    DeviceRegistry::readFromFlash();
#endif
}

bool DeviceRegistry::addDevice(const char *deviceName, const uint8_t *macPtr)
{
    if (table.find(deviceName) != table.end())
    {
        return false; // Device already exists
    }

    MacArray macArray;
    memcpy(macArray.data(), macPtr, 6);
    table[deviceName] = macArray;
    return true;
}

bool DeviceRegistry::removeDevice(const char *deviceName)
{
    return table.erase(deviceName) > 0;
}

const uint8_t *DeviceRegistry::getDeviceMac(const char *deviceName) const
{
    if (table.find(deviceName) == table.end())
    {
        return nullptr; // Device not found
    }
    return table.at(deviceName).data();
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
        table.clear();
    }
    prefs.end();
#endif
}