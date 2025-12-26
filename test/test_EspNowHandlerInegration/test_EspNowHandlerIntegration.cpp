#ifndef TEST_ESPNOWHANDLERINTEGRATION_H
#define TEST_ESPNOWHANDLERINTEGRATION_H

#include <EspNowHandler.h>
#include <cstring>
#include <unity.h>

enum class TestPacketType : uint8_t { TYPE_1, TYPE_2, Count };

enum class TestDeviceID : uint8_t { DEVICE_1, DEVICE_2, SELF, Count };

const TestDeviceID selfID = TestDeviceID::SELF;
const uint8_t selfMac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};

// Struct payload used for struct-based callback/send tests
struct TestPacketStruct {
  uint8_t command;
  uint16_t value;
  uint8_t flags;
};

// Shared state for assertions in struct-callback tests
static bool structCallbackCalled = false;
static TestPacketStruct receivedStruct = {};
static TestDeviceID receivedSender = TestDeviceID::SELF;

class EspNowHandlerTest {
public:
  friend class EspNowHandler<TestDeviceID, TestPacketType>;

  static void setUp() {
    // Setup code before each test
  }

  static void tearDown() {
    // Cleanup code after each test
  }
  static void test_CallbackGetsCalledWhenSimulatingDataReceive() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);
    const TestDeviceID senderID = TestDeviceID::DEVICE_2;
    const uint8_t senderMac[6] = {0, 1, 2, 3, 4, 5};
    const uint8_t data = 0xAC;

    auto callback = [&](const uint8_t *dataPtr, size_t len,
                        TestDeviceID sender) {
      TEST_ASSERT_EQUAL_UINT8(*dataPtr, data);
      TEST_ASSERT_EQUAL(sizeof(data), len);
      TEST_ASSERT_EQUAL(static_cast<uint8_t>(senderID),
                        static_cast<uint8_t>(sender));
      TEST_PASS();
    };

    handler.registerCallback(TestPacketType::TYPE_1, callback);

    // Create a proper packet with header (len must be size_t to match handler)
    struct PacketHeaderLike {
      uint8_t type;
      TestDeviceID sender;
      size_t len;
    } header = {static_cast<uint8_t>(TestPacketType::TYPE_1), senderID,
                sizeof(uint8_t)};

    const size_t packetSize = sizeof(header) + sizeof(uint8_t);
    uint8_t buffer[packetSize] = {};
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), &data, sizeof(uint8_t));

    handler.onDataRecv(senderMac, buffer, static_cast<int>(packetSize));
  }

  static void test_PairingWithInjectedResponse() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    // Avoid triggering real ESP-NOW sends in tests; directly inject discovery
    using Handler = EspNowHandler<TestDeviceID, TestPacketType>;

    const TestDeviceID senderID = TestDeviceID::DEVICE_2;
    const uint8_t senderMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    // Build Discovery packet payload with state=Paired to avoid ACK send
    typename Handler::PairingState state = Handler::PairingState::Paired;
    uint8_t fields[3] = {static_cast<uint8_t>(senderID),
                         static_cast<uint8_t>(selfID),
                         static_cast<uint8_t>(state)};
    const uint8_t checksum = Handler::calcChecksum(fields, sizeof(fields));

    typename Handler::DiscoveryPacket discovery{senderID, selfID, state,
                                                checksum};

    // Build header with InternalPacket::Discovery encoded type
    typename Handler::PacketType dType(Handler::InternalPacket::Discovery);
    struct PacketHeaderLike {
      uint8_t type;
      TestDeviceID sender;
      size_t len;
    } header = {dType.encoded, senderID, sizeof(discovery)};

    const size_t packetSize = sizeof(header) + sizeof(discovery);
    uint8_t buffer[packetSize] = {};
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), &discovery, sizeof(discovery));

    // Simulate receive of discovery
    handler.onDataRecv(senderMac, buffer, static_cast<int>(packetSize));

    // Validate registry updated and pairing state set
    const uint8_t *mac = handler.registry->getDeviceMac(senderID);
    TEST_ASSERT_NOT_NULL(mac);
    TEST_ASSERT_EQUAL_UINT8(senderMac[0], mac[0]);
    TEST_ASSERT_EQUAL_UINT8(senderMac[1], mac[1]);
    TEST_ASSERT_EQUAL_UINT8(senderMac[2], mac[2]);
    TEST_ASSERT_EQUAL_UINT8(senderMac[3], mac[3]);
    TEST_ASSERT_EQUAL_UINT8(senderMac[4], mac[4]);
    TEST_ASSERT_EQUAL_UINT8(senderMac[5], mac[5]);

    auto stateAfter = handler.pairingState.load();
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Handler::PairingState::Paired),
                      static_cast<uint8_t>(stateAfter));
  }

  // New: struct-based callback integration – simulate a full packet receive
  static void test_StructCallbackGetsCalledWhenSimulatingDataReceive() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    // Reset shared state
    structCallbackCalled = false;
    receivedStruct = {};
    receivedSender = TestDeviceID::SELF;

    const TestDeviceID senderID = TestDeviceID::DEVICE_2;
    const uint8_t senderMac[6] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};

    // Register struct-based callback for TYPE_1
    handler.registerCallback<TestPacketStruct>(
        TestPacketType::TYPE_1,
        [](const TestPacketStruct &data, TestDeviceID sender) {
          structCallbackCalled = true;
          receivedStruct = data;
          receivedSender = sender;
        });

    // Build a raw packet buffer: PacketHeader + TestPacketStruct payload
    struct PacketHeaderLike {
      uint8_t type;
      TestDeviceID sender;
      size_t len;
    } header = {static_cast<uint8_t>(TestPacketType::TYPE_1), senderID,
                sizeof(TestPacketStruct)};

    TestPacketStruct payload{0x42, 0x1234, 0xAB};

    const size_t packetSize = sizeof(header) + sizeof(payload);
    uint8_t buffer[packetSize] = {};
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), &payload, sizeof(payload));

    // Simulate ESP-NOW receive
    handler.onDataRecv(senderMac, buffer, static_cast<int>(packetSize));

    // Validate callback results
    TEST_ASSERT_TRUE(structCallbackCalled);
    TEST_ASSERT_EQUAL_UINT8(0x42, receivedStruct.command);
    TEST_ASSERT_EQUAL_UINT16(0x1234, receivedStruct.value);
    TEST_ASSERT_EQUAL_UINT8(0xAB, receivedStruct.flags);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(senderID),
                      static_cast<uint8_t>(receivedSender));
  }

  // New: struct-based callback integration – wrong payload size should be
  // rejected
  static void test_StructCallbackRejectsIncorrectSize() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    bool fullyInvoked = false;
    handler.registerCallback<TestPacketStruct>(
        TestPacketType::TYPE_1,
        [&fullyInvoked](const TestPacketStruct &data, TestDeviceID sender) {
          fullyInvoked = true;
        });

    // Header declares incorrect size to simulate mismatch
    struct PacketHeaderLike {
      uint8_t type;
      TestDeviceID sender;
      size_t len;
    } badHeader = {static_cast<uint8_t>(TestPacketType::TYPE_1),
                   TestDeviceID::DEVICE_2, sizeof(TestPacketStruct) - 1};

    TestPacketStruct payload{0x55, 0x6789, 0xCD};

    const size_t packetSize = sizeof(badHeader) + sizeof(payload);
    uint8_t buffer[packetSize] = {};
    memcpy(buffer, &badHeader, sizeof(badHeader));
    memcpy(buffer + sizeof(badHeader), &payload, sizeof(payload));

    // Simulate ESP-NOW receive with size mismatch
    const uint8_t senderMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    handler.onDataRecv(senderMac, buffer, static_cast<int>(packetSize));

    // Callback should have early-returned due to size mismatch
    TEST_ASSERT_FALSE(fullyInvoked);
  }
};

void setup() {
  delay(2000);
  EspNowHandlerTest handlerTest;
  UNITY_BEGIN();
  RUN_TEST(handlerTest.test_CallbackGetsCalledWhenSimulatingDataReceive);
  RUN_TEST(handlerTest.test_PairingWithInjectedResponse);
  RUN_TEST(handlerTest.test_StructCallbackGetsCalledWhenSimulatingDataReceive);
  RUN_TEST(handlerTest.test_StructCallbackRejectsIncorrectSize);

  UNITY_END();
}
void loop() {}

#endif