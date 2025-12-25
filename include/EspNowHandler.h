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

  static HANDLER_PARAMS *instance;
  // Static instance pointer for callbacks

  bool pairDevice(DeviceID targetDeviceID, bool encrypt);
  // Pairs a specific device by sending
  // broadcasts with the target device ID
  // and the sender device ID

  bool handleDiscoveryPacket(const uint8_t *macAddrPtr, const uint8_t *dataPtr);

  void sendDiscoveryPacket(DeviceID targetID);

  static void onDataSent(const uint8_t *macAddrPtr,
                         esp_now_send_status_t status);
  static void onDataRecv(const uint8_t *macAddrPtr, const uint8_t *dataPtr,
                         int data_len);
  static constexpr size_t toIndex(PacketType packetType);
  static uint8_t calcChecksum(const uint8_t *dataPtr, size_t len);

  std::array<PacketCallback, PacketCount> packetCallbacks = {};
  DeviceID selfID;
  uint8_t selfMac[6] = {};

  friend class EspNowHandlerTest;

public:
  DeviceRegistry<DeviceID> *registry;

  EspNowHandler(DeviceID selfDeviceID, const uint8_t *selfMacPtr);
  // Initializes the class and registers
  // the given name as the own device name
  // (mainly used for pairing).

  bool begin();

  bool registerComms(DeviceID targetID, bool pairingMode = false,
                     bool encrypt = false);
  // Returns a 1-Byte integer ID for the target device to use as
  // the commID for sending packets and for identifying where
  // packets were sent from. If no device of the given name
  // exists, and pairingMode is true, enters pairing mode to try
  // and find the device

  bool registerCallback(PacketType packetTypeID, PacketCallback);
  // Registers a callback function for a specific packet
  // type. Multiple callbacks per type not possible.
  // PacketCallback must be format "void function(const
  // uint8_t dataPtr, size_t len, uint8_t sender)"

  bool sendPacket(DeviceID targetID, PacketType packetType,
                  const uint8_t *dataPtr, size_t len);
  // Sends a packet of the type "packetType" to a
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
  DeviceID senderID;
  DeviceID targetID;
  PairingState state;
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
  size_t len;
};

// Template implementation
HANDLER_TEMPLATE
HANDLER_PARAMS *HANDLER_PARAMS::instance = nullptr;

HANDLER_TEMPLATE
HANDLER_PARAMS::EspNowHandler(DeviceID selfDeviceID,
                              const uint8_t *selfMacPtr) {
  registry = new DeviceRegistry<DeviceID>(selfDeviceID, selfMacPtr);
  selfID = selfDeviceID;
  memcpy(selfMac, selfMacPtr, 6);
  instance = this; // Set static instance pointer
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::begin() {
  if (esp_now_init() != ESP_OK) {
    return false;
  }
  esp_err_t registerSentSuccess = esp_now_register_send_cb(onDataSent);
  esp_err_t regiserRecvSuccess = esp_now_register_recv_cb(onDataRecv);
  return (regiserRecvSuccess == ESP_OK) && (registerSentSuccess == ESP_OK);
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
bool HANDLER_PARAMS::registerComms(DeviceID targetID, bool pairingMode,
                                   bool encrypt) {
  const uint8_t *macPtr = registry->getDeviceMac(targetID);
  bool pair = false;
  if (macPtr == nullptr) {
    if (!pairingMode) {
      return false;
    }
    macPtr = BroadCastMac; // use broadcast when pairing
    pair = true;
  }
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macPtr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = (encrypt && !pairingMode);
  esp_err_t addPeerReturn = esp_now_add_peer(&peerInfo);
  if (addPeerReturn != ESP_OK) {
    return false;
  }
  if (pair == true)
    pairDevice(targetID, encrypt);
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
  if (targetID == selfID) {
    printf("[ESPNowHandler] Cannot send packet to self\n");
    return false; // Cannot send to self
  }
  const uint8_t *targetMac;
  if (PacketType(packetType).encoded ==
      PacketType(InternalPacket::Discovery).encoded) { // Send to broadcast
    targetMac = BroadCastMac;
  } else { // Normal send
    targetMac = this->registry->getDeviceMac(targetID);
  }
  if (targetMac == nullptr) {
    printf("[ESPNowHandler] Target MAC not found for device ID %u\n",
           static_cast<uint8_t>(targetID));
    return false;
  }

  PacketHeader packetHeader = {packetType.encoded, selfID, len};

  size_t packetSize = sizeof(dataPtr) + sizeof(packetHeader) + sizeof(len);

  uint8_t data[packetSize] = {};

  memcpy(data, &packetHeader, sizeof(PacketHeader));
  memcpy(data + sizeof(PacketHeader), dataPtr, sizeof(dataPtr));
  memcpy(data + sizeof(PacketHeader) + sizeof(&dataPtr), &len, sizeof(len));

  esp_err_t sendSuccess = esp_now_send(targetMac, data, sizeof(data));
  if (sendSuccess != ESP_OK) {
    printf("[ESPNowHandler] Failed to send packet, esp_err_t: %d\n",
           sendSuccess);
    return false;
  }
  return true;
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::pairDevice(DeviceID targetDeviceID, bool encrypt) {
  printf("[ESPNowHandler] Starting pairing with device ID %u\n",
         static_cast<uint8_t>(targetDeviceID));
  pairingState = PairingState::Waiting;
  uint8_t retries = 0;
  const uint8_t *targetMac = BroadCastMac;
  while (pairingState != PairingState::Paired && retries < maxRetries) {
    sendDiscoveryPacket(targetDeviceID);
    retries++;
    if (retries == maxRetries) {
      pairingState = PairingState::Timeout;
    }
    delay(1000);
  }

  if (pairingState == PairingState::Timeout) {
    return false;
  }
  registerComms(targetDeviceID, false, encrypt); // Re-register with actual MAC
  return true;
}

HANDLER_TEMPLATE
void HANDLER_PARAMS::onDataRecv(const uint8_t *macAddrPtr,
                                const uint8_t *dataPtr, int data_len) {
  // printf("Data received\n");
  if (!instance) {
    printf("[ESPNowHandler] Instance is null\n");
    return; // Safety check
  }

  if (data_len < sizeof(PacketHeader)) {
    printf("[ESPNowHandler] Data length too small: %d\n", data_len);
    return; // Not enough data for header
  }

  PacketHeader header;
  memcpy(&header, dataPtr, sizeof(PacketHeader));

  if (header.type == PacketType(InternalPacket::Discovery).encoded) {
    instance->handleDiscoveryPacket(macAddrPtr, dataPtr);
    return;
  }

  // Bounds check for callback array
  if (header.type >= PacketCount) {
    printf("[ESPNowHandler] Header type out of bounds: %d\n", header.type);
    return;
  }

  // Check if callback is registered
  if (!instance->packetCallbacks[header.type]) {
    printf("[ESPNowHandler] No callback registered for header type: %d\n",
           header.type);
    return;
  }

  // Pass data after the header to the callback
  const uint8_t *payloadPtr = dataPtr + sizeof(PacketHeader);
  instance->packetCallbacks[header.type](payloadPtr, header.len, header.sender);
}

HANDLER_TEMPLATE
void HANDLER_PARAMS::onDataSent(const uint8_t *macAddrPtr,
                                esp_now_send_status_t status) {
  return; // Possibly implement something here later
}

HANDLER_TEMPLATE
bool HANDLER_PARAMS::handleDiscoveryPacket(const uint8_t *macAddrPtr,
                                           const uint8_t *dataPtr) {
  DiscoveryPacket packet;
  memcpy(&packet, dataPtr + sizeof(PacketHeader), sizeof(DiscoveryPacket));

  uint8_t fields[3] = {static_cast<uint8_t>(packet.senderID),
                       static_cast<uint8_t>(packet.targetID),
                       static_cast<uint8_t>(packet.state)};

  const uint8_t checksum = calcChecksum(fields, sizeof(fields));

  if (checksum != packet.checksum) {
    printf("[ESPNowHandler] Invalid checksum in discovery packet\n");
    return false; // Invalid checksum
  }

  if (packet.targetID != selfID) {
    printf("[ESPNowHandler] Discovery packet not for us (target ID %u)\n",
           static_cast<uint8_t>(packet.targetID));
    return false; // Not for us
  }

  printf("[ESPNowHandler] Adding device ID %u with MAC "
         "%02X:%02X:%02X:%02X:%02X:%02X\n",
         static_cast<uint8_t>(packet.senderID), macAddrPtr[0], macAddrPtr[1],
         macAddrPtr[2], macAddrPtr[3], macAddrPtr[4], macAddrPtr[5]);
  bool addSuccess = registry->addDevice(packet.senderID, macAddrPtr);
  if (addSuccess)
    registry->saveToFlash();
  printf("[ESPNowHandler] External device registration: %s\n",
         addSuccess ? "success" : "failure");
  pairingState = PairingState::Paired;
  if (packet.state == PairingState::Waiting)
    sendDiscoveryPacket(packet.senderID); // Acknowledge by sending back
  return addSuccess;
}

HANDLER_TEMPLATE
void HANDLER_PARAMS::sendDiscoveryPacket(DeviceID targetDeviceID) {
  PairingState pairingStateLocal = pairingState.load();
  uint8_t fields[3] = {static_cast<uint8_t>(selfID),
                       static_cast<uint8_t>(targetDeviceID),
                       static_cast<uint8_t>(pairingStateLocal)};

  const uint8_t checksum = calcChecksum(fields, sizeof(fields));

  DiscoveryPacket discoveryPacket = {selfID, targetDeviceID, pairingStateLocal,
                                     checksum};

  const size_t packetSize = sizeof(DiscoveryPacket);

  uint8_t data[packetSize] = {};
  memcpy(data, &discoveryPacket, sizeof(DiscoveryPacket));
  sendPacket(targetDeviceID, InternalPacket::Discovery, data, sizeof(data));
}

#endif