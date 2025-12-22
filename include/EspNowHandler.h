#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

#include <DeviceRegistry.h>

class EspNowHandler
{
public:
    uint8_t registerComms(const char *targetName, bool pairingMode = false); // Returns a 1-Byte integer ID to use as the CommID for sending packets to a device. If no device of the given name exists, and pairingMode is true, enters pairing mode to try and find the device

    bool registerPacketTypeCb(const char *packetType, void *callbackFunction); // Registers a callback function for a specific packet type. Multiple callbacks per type possible

    bool sendPacket(uint8_t commID,
                    const char *packetType,
                    const uint8_t *dataPtr,
                    size_t len); // Sends a packet of the type "packetType" to a device with the corresponding commID (as returned when calling registerComms)
};

#endif