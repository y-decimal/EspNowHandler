#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

#include <DeviceRegistry.h>
#include <esp_now.h>
#include <atomic>

using PacketCallback = std::function<void(const uint8_t *dataPtr, size_t len, uint8_t sender)>;

#define ESP_NOW_HANDLER_TEMPLATE template <typename DeviceID, typename UserPacket, size_t DeviceCount, size_t PacketCount>

ESP_NOW_HANDLER_TEMPLATE
class EspNowHandler
{
private:
    struct PacketType;
    struct PacketHeader;
    struct DiscoveryPacket;
    enum class PairingState : uint8_t;
    enum class InternalPacket : uint8_t;

    bool pairDevice(DeviceID targetDeviceID); // Pairs a specific device by sending broadcasts with the target device ID and the sender device ID

    static void onDataSent(const uint8_t *macAddrPtr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *macAddrPtr, const uint8_t *dataPtr, int data_len);
    static constexpr size_t toIndex(PacketType packetType);
    static constexpr uint8_t maxRetries = 30;
    static const uint8_t calcChecksum(const uint8_t *dataPtr);

    DeviceRegistry<DeviceID, DeviceCount> *registry;
    std::array<PacketCallback, PacketCount> packetCallbacks = {};
    DeviceID selfID;
    volatile std::atomic<PairingState> pairingState = PairingState::Waiting;

    friend class EspNowHandlerTest;

    struct PacketHeader
    {
        uint8_t type;
        uint16_t len;
    };

    struct PacketType
    {
        uint8_t encoded;

        PacketType(UserPacket packet)
            : encoded(static_cast<uint8_t>(packet)) {}

        PacketType(InternalPacket packet)
            : encoded(128 + static_cast<uint8_t>(packet)) {}
    };

    struct DiscoveryPacket
    {
        uint8_t sequence;
        DeviceID senderID;
        DeviceID targetID;
        uint8_t checksum;
    };

    enum class PairingState : uint8_t
    {
        Waiting,
        Paired,
        Timeout
    };

    enum class InternalPacket : uint8_t
    {
        Discovery,
        Count
    };

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

#endif