#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

#include <DeviceRegistry.h>

using PacketCallback = std::function<void(const uint8_t *dataPtr, size_t len, uint8_t sender)>;

class EspNowHandler
{
private:
    bool pairDevice(uint8_t targetDeviceID); // Pairs a specific device by sending broadcasts with the target device ID and the sender device ID, the receiving side checks if the device ID matches its own ID, and if it does saves the sender device ID and MAC to its registry, disables discovery mode if active, then sends an identical packet to the sender specifically. The sender receives the answer, saves the device ID and MAC to its registry, then disables discovery mode if active.

public:
    EspNowHandler(uint8_t selfDeviceID); // Initializes the class and registers the given ID as the own device ID (mainly used for pairing).

    bool registerComms(uint8_t targetDeviceID, bool pairingMode = false); // If no device of the given ID exists in the registry, and pairingMode is true, enters pairing mode to try and find the device

    bool registerCallback(uint8_t packetTypeID, PacketCallback); // Registers a callback function for a specific packet type. Multiple callbacks per type not possible. PacketCallback must be format "void function(const uint8_t dataPtr, size_t len, uint8_t sender)"

    bool sendPacket(uint8_t targetDeviceID,
                    uint8_t packetTypeID,
                    const uint8_t *dataPtr,
                    size_t len); // Sends a packet of the type "packetTypeID" to a device with the corresponding targetDeviceID
};

#endif