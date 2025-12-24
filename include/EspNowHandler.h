#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

#include <DeviceRegistry.h>
#include <esp_now.h>
#include <atomic>

using PacketCallback = std::function<void(const uint8_t *dataPtr, size_t len, uint8_t sender)>;

#define HANDLER_TEMPLATE template <typename DeviceID, typename UserPacket>

#define HANDLER_PARAMS EspNowHandler<DeviceID, UserPacket>

HANDLER_TEMPLATE
class EspNowHandler
{
private:
    struct PacketType;
    struct PacketHeader;
    struct DiscoveryPacket;
    enum class PairingState : uint8_t;
    enum class InternalPacket : uint8_t;
    volatile std::atomic<PairingState> pairingState = PairingState::Waiting;

    static constexpr uint8_t maxRetries = 30;
    static constexpr size_t DeviceCount = static_cast<size_t>(DeviceID::Count);
    static constexpr size_t PacketCount = static_cast<size_t>(UserPacket::Count);

    bool pairDevice(DeviceID targetDeviceID); // Pairs a specific device by sending broadcasts with the target device ID and the sender device ID

    static void onDataSent(const uint8_t *macAddrPtr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *macAddrPtr, const uint8_t *dataPtr, int data_len);
    static constexpr size_t toIndex(PacketType packetType);
    static constexpr uint8_t calcChecksum(const uint8_t *dataPtr);

    DeviceRegistry<DeviceID, DeviceCount> *registry;
    std::array<PacketCallback, PacketCount> packetCallbacks = {};
    DeviceID selfID;

    friend class EspNowHandlerTest;

public:
    EspNowHandler(DeviceID selfDeviceID); // Initializes the class and registers the given name as the own device name (mainly used for pairing).

    bool begin();

    bool registerComms(DeviceID targetID, bool pairingMode = false); // Returns a 1-Byte integer ID for the target device to use as the commID for sending packets and for identifying where packets were sent from. If no device of the given name exists, and pairingMode is true, enters pairing mode to try and find the device

    bool registerCallback(PacketType packetTypeID, PacketCallback); // Registers a callback function for a specific packet type. Multiple callbacks per type not possible. PacketCallback must be format "void function(const uint8_t dataPtr, size_t len, uint8_t sender)"

    bool sendPacket(DeviceID targetID,
                    PacketType packetType,
                    const uint8_t *dataPtr,
                    size_t len); // Sends a packet of the type "packetType" to a device with the corresponding commID (as returned when calling registerComms)
};

// Full definitions

HANDLER_TEMPLATE
struct HANDLER_PARAMS::PacketType
{
    uint8_t encoded;

    PacketType(UserPacket packet)
        : encoded(static_cast<uint8_t>(packet)) {}

    PacketType(InternalPacket packet)
        : encoded(128 + static_cast<uint8_t>(packet)) {}
};

HANDLER_TEMPLATE
struct HANDLER_PARAMS::DiscoveryPacket
{
    uint8_t sequence;
    DeviceID senderID;
    DeviceID targetID;
    uint8_t checksum;
};

HANDLER_TEMPLATE
enum class HANDLER_PARAMS::PairingState : uint8_t
{
    Waiting,
    Paired,
    Timeout
};

HANDLER_TEMPLATE
enum class HANDLER_PARAMS::InternalPacket : uint8_t
{
    Discovery,
    Count
};

struct PacketHeader
{
    uint8_t type;
    uint16_t len;
};

// Template implementation
HANDLER_TEMPLATE
HANDLER_PARAMS::EspNowHandler(DeviceID selfDeviceID)
{
    registry = new DeviceRegistry<DeviceID, DeviceCount>();
    selfID = selfDeviceID;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::begin()
{
    if (esp_now_init() != ESP_OK)
    {
        return false;
    }
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);
    return true;
}

HANDLER_TEMPLATE
constexpr size_t HANDLER_PARAMS::toIndex(PacketType packetType)
{
    return static_cast<size_t>(packetType);
}

HANDLER_TEMPLATE
constexpr uint8_t HANDLER_PARAMS::calcChecksum(const uint8_t *dataPtr)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < sizeof(dataPtr); ++i)
    {
        checksum ^= dataPtr[i];
    }
    return checksum;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::registerComms(DeviceID targetID, bool pairingMode)
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

HANDLER_TEMPLATE
bool HANDLER_PARAMS::registerCallback(PacketType packetType, PacketCallback callback)
{
    packetCallbacks[toIndex(packetType)] = callback;
    return true;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::sendPacket(DeviceID targetID, PacketType packetType,
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

HANDLER_TEMPLATE
bool HANDLER_PARAMS::pairDevice(DeviceID targerDeviceID)
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
            static_cast<PacketHeader>(DiscoveryType)};
    }

    if (pairingState == PairingState::Timeout)
    {
        return false;
    }
    return true;
}

#endif