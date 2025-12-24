#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

#include <DeviceRegistry.h>
#include <atomic>
#include <esp_now.h>
#include <type_traits>

#define HANDLER_TEMPLATE template <typename DeviceID, typename UserPacket>

#define HANDLER_PARAMS EspNowHandler<DeviceID, UserPacket>

HANDLER_TEMPLATE
class EspNowHandler {
private:
  using PacketCallback =
      std::function<void(const uint8_t *dataPtr, size_t len, DeviceID sender)>;

  static_assert(std::is_enum<UserPacket>::value,
                "UserPacket must be an enum type");
  static_assert(std::is_same<typename std::underlying_type<UserPacket>::type,
                             uint8_t>::value,
                "UserPacket underlying type must be uint8_t");

  struct PacketType;
  struct PacketHeader;
  struct DiscoveryPacket;
  enum class PairingState : uint8_t;
  enum class InternalPacket : uint8_t;
  std::atomic<PairingState> pairingState{PairingState::Waiting};

  static constexpr uint8_t maxRetries = 30;
  static constexpr size_t DeviceCount = static_cast<size_t>(DeviceID::Count);
  static constexpr size_t PacketCount = static_cast<size_t>(UserPacket::Count);

  static HANDLER_PARAMS *instance; // Static instance pointer for callbacks

  bool
  pairDevice(DeviceID targetDeviceID); // Pairs a specific device by sending
                                       // broadcasts with the target device ID
                                       // and the sender device ID

  static void onDataSent(const uint8_t *macAddrPtr,
                         esp_now_send_status_t status);
  static void onDataRecv(const uint8_t *macAddrPtr, const uint8_t *dataPtr,
                         int data_len);
  static constexpr size_t toIndex(PacketType packetType);
  static uint8_t calcChecksum(const uint8_t *dataPtr, size_t len);

  DeviceRegistry<DeviceID> *registry;
  std::array<PacketCallback, PacketCount> packetCallbacks = {};
  DeviceID selfID;

  friend class EspNowHandlerTest;

public:
  EspNowHandler(DeviceID selfDeviceID); // Initializes the class and registers
                                        // the given name as the own device name
                                        // (mainly used for pairing).

  bool begin();

  bool registerComms(
      DeviceID targetID,
      bool pairingMode =
          false); // Returns a 1-Byte integer ID for the target device to use as
                  // the commID for sending packets and for identifying where
                  // packets were sent from. If no device of the given name
                  // exists, and pairingMode is true, enters pairing mode to try
                  // and find the device

  bool registerCallback(
      PacketType packetTypeID,
      PacketCallback); // Registers a callback function for a specific packet
                       // type. Multiple callbacks per type not possible.
                       // PacketCallback must be format "void function(const
                       // uint8_t dataPtr, size_t len, uint8_t sender)"

  bool sendPacket(DeviceID targetID, PacketType packetType,
                  const uint8_t *dataPtr,
                  size_t len); // Sends a packet of the type "packetType" to a
                               // device with the corresponding commID (as
                               // returned when calling registerComms)
};

// Full definitions

HANDLER_TEMPLATE
struct HANDLER_PARAMS::PacketType {
  uint8_t encoded;

  PacketType(UserPacket packet) : encoded(static_cast<uint8_t>(packet)) {}

  PacketType(InternalPacket packet)
      : encoded(128 + static_cast<uint8_t>(packet)) {}
};

HANDLER_TEMPLATE
struct HANDLER_PARAMS::DiscoveryPacket {
  uint8_t sequence;
  DeviceID senderID;
  DeviceID targetID;
  uint8_t checksum;
};

HANDLER_TEMPLATE
enum class HANDLER_PARAMS::PairingState : uint8_t { Waiting, Paired, Timeout };

HANDLER_TEMPLATE
enum class HANDLER_PARAMS::InternalPacket : uint8_t { Discovery, Count };

HANDLER_TEMPLATE
struct HANDLER_PARAMS::PacketHeader {
  uint8_t type;
  DeviceID sender;
  uint16_t len;
};

// Template implementation
HANDLER_TEMPLATE
HANDLER_PARAMS *HANDLER_PARAMS::instance = nullptr;

HANDLER_TEMPLATE
HANDLER_PARAMS::EspNowHandler(DeviceID selfDeviceID) {
  registry = new DeviceRegistry<DeviceID>();
  selfID = selfDeviceID;
  instance = this; // Set static instance pointer
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::begin() {
  if (esp_now_init() != ESP_OK) {
    return false;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  return true;
}

HANDLER_TEMPLATE
constexpr size_t HANDLER_PARAMS::toIndex(PacketType packetType) {
  return static_cast<size_t>(packetType.encoded);
}

HANDLER_TEMPLATE
uint8_t HANDLER_PARAMS::calcChecksum(const uint8_t *dataPtr, size_t len) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < len; ++i) {
    checksum ^= dataPtr[i];
  }
  return checksum;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::registerComms(DeviceID targetID, bool pairingMode) {
  const uint8_t *macPtr = registry->getDeviceMac(targetID);
  if (macPtr == nullptr) {
    if (!pairingMode) {
      return false;
    }
    macPtr = BroadCastMac; // use broadcast when pairing
  }
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macPtr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_err_t addPeerReturn = esp_now_add_peer(&peerInfo);
  if (addPeerReturn != ESP_OK) {
    return false;
  }
  return true;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::registerCallback(PacketType packetType,
                                      PacketCallback callback) {
  packetCallbacks[toIndex(packetType)] = callback;
  return true;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::sendPacket(DeviceID targetID, PacketType packetType,
                                const uint8_t *dataPtr, size_t len) {
  const uint8_t *targetMac = this->registry->getDeviceMac(targetID);
  if (targetMac == nullptr) {
    return false;
  }
  PacketHeader packetHeader = {packetType.encoded, len};
  const uint8_t *data = packetHeader + dataPtr;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::pairDevice(DeviceID targerDeviceID) {
  pairingState = PairingState::Waiting;
  uint8_t retries = 0;
  while (pairingState == PairingState::Waiting && retries < maxRetries) {
    const uint8_t *targetMac = BroadCastMac;
    uint8_t fields[3] = {retries, static_cast<uint8_t>(selfID),
                         static_cast<uint8_t>(targerDeviceID)};
    const uint8_t checksum = calcChecksum(fields, sizeof(fields));

    DiscoveryPacket discoveryPacket = {retries, selfID, targerDeviceID,
                                       checksum};

    uint8_t DiscoveryType = 0xFF;

    PacketHeader packetHeader = {static_cast<PacketHeader>(DiscoveryType)};
  }

  if (pairingState == PairingState::Timeout) {
    return false;
  }
  return true;
}

#endif