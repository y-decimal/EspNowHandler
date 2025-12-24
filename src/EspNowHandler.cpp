#include <EspNowHandler.h>

ESP_NOW_HANDLER_TEMPLATE
EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
    EspNowHandler(DeviceID selfDeviceID)
{
    registry = new DeviceRegistry<DeviceID, DeviceCount>();
    selfID = selfDeviceID;
}

ESP_NOW_HANDLER_TEMPLATE
bool EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
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
constexpr size_t EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
    toIndex(PacketType packetType)
{
    return static_cast<size_t>(packetType);
}

ESP_NOW_HANDLER_TEMPLATE
const uint8_t EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
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
bool EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
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
bool EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
    registerCallback(PacketType packetType, PacketCallback callback)
{
    packetCallbacks[toIndex(packetType)] = callback;
    return true;
}

ESP_NOW_HANDLER_TEMPLATE
bool EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
    sendPacket(DeviceID targetID, PacketType packetType,
               const uint8_t *dataPtr, size_t len)
{
    const uint8_t *targetMac = this->registry->getDeviceMac(targetID);
    if (targetMac == nullptr)
    {
        return false;
    }
    PacketHeader packetHeader = {packetType.encoded, len};
    const uint8_t *data = packetHeader + dataPtr;
}

ESP_NOW_HANDLER_TEMPLATE
bool EspNowHandler<DeviceID, UserPacket, DeviceCount, PacketCount>::
    pairDevice(DeviceID targerDeviceID)
{
    pairingState = PairingState::Waiting;
    uint8_t retries = 0;
    while (pairingState == PairingState::Waiting && retries < maxRetries)
    {
        const uint8_t *targetMac = BroadCastMac;

        const uint8_t checksum = calcChecksum(retries + selfID + targerDeviceID);

        DiscoveryPacket discoveryPacket = {
            retries,
            selfID,
            targerDeviceID,
            checksum};

        uint8_t DiscoveryType = 0xFF;

        PacketHeader packetHeader = {
            static_cast<PacketHeader>(DiscoveryType)
    };
}

if (pairingState == PairingState::Timeout)
{
    return false;
}
return true;
}
