#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

#include <DeviceRegistry.h>

using PacketCallback = std::function<void(const uint8_t *dataPtr, size_t len, uint8_t sender)>;

class EspNowHandler
{
private: 


public:
    EspNowHandler(const char *selfDeviceName); // Initializes the class and registers the given name as the own device name (mainly used for pairing).

    uint8_t registerComms(const char *targetName, bool pairingMode = false); // Returns a 1-Byte integer ID for the target device to use as the commID for sending packets and for identifying where packets were sent from. If no device of the given name exists, and pairingMode is true, enters pairing mode to try and find the device

    bool registerCallback(uint8_t packetTypeID, PacketCallback); // Registers a callback function for a specific packet type. Multiple callbacks per type not possible. PacketCallback must be format "void function(const uint8_t dataPtr, size_t len, uint8_t sender)"

    bool sendPacket(uint8_t commID,
                    uint8_t packetTypeID,
                    const uint8_t *dataPtr,
                    size_t len); // Sends a packet of the type "packetType" to a device with the corresponding commID (as returned when calling registerComms)
};

#endif