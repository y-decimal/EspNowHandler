#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

#include <DeviceRegistry.h>
#include <esp_now.h>

using PacketCallback = std::function<void(const uint8_t *dataPtr, size_t len, uint8_t sender)>;

#define ESP_NOW_HANDLER_TEMPLATE template <typename DeviceID, typename PacketType, size_t DeviceCount, size_t PacketCount>

ESP_NOW_HANDLER_TEMPLATE
class EspNowHandler
{
private:
    bool pairDevice(DeviceID targetDeviceID); // Pairs a specific device by sending broadcasts with the target device ID and the sender device ID

    static void onDataSent(const uint8_t *macAddrPtr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *macAddrPtr, const uint8_t *dataPtr, int data_len);
    static constexpr size_t toIndex(PacketType packetType);
    static const uint8_t calcChecksum(const uint8_t *dataPtr);

    struct PacketHeader
    {
        PacketType type;
        size_t len;
        uint8_t checksum;
    };

    DeviceRegistry<DeviceID, DeviceCount> *registry;
    std::array<PacketCallback, PacketCount> packetCallbacks = {};

    friend class EspNowHandlerTest;

public:
    EspNowHandler(DeviceID selfDeviceID); // Initializes the class and registers the given name as the own device name (mainly used for pairing).

    bool begin();

    bool registerComms(DeviceID targetID, bool pairingMode = false); // Returns a 1-Byte integer ID for the target device to use as the commID for sending packets and for identifying where packets were sent from. If no device of the given name exists, and pairingMode is true, enters pairing mode to try and find the device

    bool registerCallback(PacketType packetTypeID, PacketCallback); // Registers a callback function for a specific packet type. Multiple callbacks per type not possible. PacketCallback must be format "void function(const uint8_t dataPtr, size_t len, uint8_t sender)"

    bool sendPacket(DeviceID targetID,
                    PacketType packetTypeID,
                    const uint8_t *dataPtr,
                    size_t len); // Sends a packet of the type "packetType" to a device with the corresponding commID (as returned when calling registerComms)
};

ESP_NOW_HANDLER_TEMPLATE
EspNowHandler<DeviceID, PacketType, DeviceCount, PacketCount>::
    EspNowHandler(DeviceID selfDeviceID)
{
    registry = new DeviceRegistry<DeviceID, DeviceCount>();
}

ESP_NOW_HANDLER_TEMPLATE
bool EspNowHandler<DeviceID, PacketType, DeviceCount, PacketCount>::
    begin()
{
    if (esp_now_init() != ESP_OK)
    {
        return false;
    }
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);
    return true;
}

ESP_NOW_HANDLER_TEMPLATE
constexpr size_t EspNowHandler<DeviceID, PacketType, DeviceCount, PacketCount>::
    toIndex(PacketType packetType)
{
    return static_cast<size_t>(packetType);
}

ESP_NOW_HANDLER_TEMPLATE
const uint8_t EspNowHandler<DeviceID, PacketType, DeviceCount, PacketCount>::
    calcChecksum(const uint8_t *dataPtr)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < sizeof(dataPtr); ++i)
    {
        checksum ^= dataPtr[i];
    }
    return checksum;
}

ESP_NOW_HANDLER_TEMPLATE
bool EspNowHandler<DeviceID, PacketType, DeviceCount, PacketCount>::
    registerComms(DeviceID targetID, bool pairingMode)
{
    const uint8_t targetMac[6] = {};
    memcpy(targetMac, registry->getDeviceMac(targetID), 6);
    if (targetMac == nullptr)
    {
        return false; // implement pairing here later
    }
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, targetMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_err_t addPeerReturn = esp_now_add_peer(&peerInfo);
    if (addPeerReturn != ESP_OK)
    {
        return false;
    }
    return true;
}

ESP_NOW_HANDLER_TEMPLATE
bool EspNowHandler<DeviceID, PacketType, DeviceCount, PacketCount>::
    registerCallback(PacketType packetTypeID, PacketCallback callback)
{
    packetCallbacks[toIndex(packetTypeID)] = callback;
    return true;
}

ESP_NOW_HANDLER_TEMPLATE
bool EspNowHandler<DeviceID, PacketType, DeviceCount, PacketCount>::
    sendPacket(DeviceID targetID, PacketType packetTypeID,
               const uint8_t *dataPtr, size_t len)
{
    const uint8_t *targetMac = this->registry->getMac(targetID);
    if (targetMac == nullptr)
    {
        return false;
    }
    const uint8_t checksum = calcChecksum(dataPtr);
    PacketHeader packetHeader = {packetTypeID, len, checksum};
    const uint8_t *data = packetHeader + dataPtr;
}

#endif