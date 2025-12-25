#ifndef TEST_ESPNOWHANDLERINTEGRATION_H
#define TEST_ESPNOWHANDLERINTEGRATION_H

#include <EspNowHandler.h>
#include <cstring>
#include <unity.h>

enum class TestPacketType : uint8_t { TYPE_1, TYPE_2, Count };

enum class TestDeviceID : uint8_t { DEVICE_1, DEVICE_2, SELF, Count };

const TestDeviceID selfID = TestDeviceID::SELF;
const uint8_t selfMac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};

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

    // Create a proper packet with header
    struct {
      uint8_t type;
      TestDeviceID sender;
      uint16_t len;
      uint8_t payload;
    } packet = {static_cast<uint8_t>(TestPacketType::TYPE_1), senderID,
                sizeof(data), data};

    handler.onDataRecv(senderMac, reinterpret_cast<const uint8_t *>(&packet),
                       sizeof(packet));
  }

  static void test_PairingWithInjectedResponse() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    bool pairingSuccess = handler.registerComms(TestDeviceID::DEVICE_2, true);
  }
};

void setup() {
  delay(2000);
  EspNowHandlerTest handlerTest;
  UNITY_BEGIN();

  UNITY_END();
}
void loop() {}

#endif