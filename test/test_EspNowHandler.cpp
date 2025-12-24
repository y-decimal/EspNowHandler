#ifndef TEST_ESPNOWHANDLER_H
#define TEST_ESPNOWHANDLER_H

#include <EspNowHandler.h>
#include <cstring>
#include <unity.h>

enum class TestPacketType : uint8_t { TYPE_1, TYPE_2, Count };

enum class TestDeviceID : uint8_t { DEVICE_1, DEVICE_2, Count };

class EspNowHandlerTest {
public:
  friend class EspNowHandler<TestDeviceID, TestPacketType>;

  static void setUp() {
    // Setup code before each test
  }

  static void tearDown() {
    // Cleanup code after each test
  }

  static void test_toIndex_convertsPacketTypeToSize() {
    size_t index0 = EspNowHandler<TestDeviceID, TestPacketType>::toIndex(
        TestPacketType::TYPE_1);
    size_t index1 = EspNowHandler<TestDeviceID, TestPacketType>::toIndex(
        TestPacketType::TYPE_2);

    TEST_ASSERT_EQUAL(0, index0);
    TEST_ASSERT_EQUAL(1, index1);
  }

  static void test_calcChecksum_returnsCorrectChecksum() {
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t checksum =
        EspNowHandler<TestDeviceID, TestPacketType>::calcChecksum(
            testData, sizeof(testData));
    uint8_t expected = 0x01 ^ 0x02 ^ 0x03 ^ 0x04;

    TEST_ASSERT_EQUAL(expected, checksum);
  }

  static void test_registerCallback_storesCallback() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(TestDeviceID::DEVICE_1);
    bool result = handler.registerCallback(
        TestPacketType::TYPE_1,
        [](const uint8_t *data, size_t len, TestDeviceID sender) {});

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(handler.packetCallbacks[0]);
  }

  static void test_constructor_initializesRegistry() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(TestDeviceID::DEVICE_1);

    TEST_ASSERT_NOT_NULL(handler.registry);
  }

  static void test_packetCallbacksArray_isInitializedEmpty() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(TestDeviceID::DEVICE_1);

    TEST_ASSERT_NULL(handler.packetCallbacks[0]);
    TEST_ASSERT_NULL(handler.packetCallbacks[1]);
  }

  static void test_CallbackGetsCalledWhenSimulatingDataReceive() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(TestDeviceID::DEVICE_1);
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
};

void setup() {
  delay(2000);
  EspNowHandlerTest handlerTest;
  UNITY_BEGIN();
  RUN_TEST(handlerTest.test_calcChecksum_returnsCorrectChecksum);
  RUN_TEST(handlerTest.test_constructor_initializesRegistry);
  RUN_TEST(handlerTest.test_packetCallbacksArray_isInitializedEmpty);
  RUN_TEST(handlerTest.test_registerCallback_storesCallback);
  RUN_TEST(handlerTest.test_toIndex_convertsPacketTypeToSize);
  RUN_TEST(handlerTest.test_CallbackGetsCalledWhenSimulatingDataReceive);
  UNITY_END();
}
void loop() {}

#endif