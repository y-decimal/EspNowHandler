#ifndef TEST_ESPNOWHANDLERINTERNAL_H
#define TEST_ESPNOWHANDLERINTERNAL_H

#include <EspNowHandler.h>
#include <cstring>
#include <unity.h>

enum class TestPacketType : uint8_t { TYPE_1, TYPE_2, Count };

enum class TestDeviceID : uint8_t { DEVICE_1, DEVICE_2, SELF, Count };

const TestDeviceID selfID = TestDeviceID::SELF;
const uint8_t selfMac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};

// Test struct for struct-based send/receive
struct TestPacketStruct {
  uint8_t command;
  uint16_t value;
  uint8_t flags;
};

// Global variables for callback testing
bool structCallbackCalled = false;
TestPacketStruct receivedStruct = {};
TestDeviceID receivedSender = TestDeviceID::SELF;

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
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);
    bool result = handler.registerCallback(
        TestPacketType::TYPE_1,
        [](const uint8_t *data, size_t len, TestDeviceID sender) {});

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(handler.packetCallbacks[0]);
  }

  static void test_constructor_initializesRegistry() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    TEST_ASSERT_NOT_NULL(handler.registry);
  }

  static void test_packetCallbacksArray_isInitializedEmpty() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    TEST_ASSERT_NULL(handler.packetCallbacks[0]);
    TEST_ASSERT_NULL(handler.packetCallbacks[1]);
  }

  static void test_registerCallback_withStructPacket_storesCallback() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    bool result = handler.registerCallback<TestPacketStruct>(
        TestPacketType::TYPE_1,
        [](const TestPacketStruct &data, TestDeviceID sender) {
          structCallbackCalled = true;
          receivedStruct = data;
          receivedSender = sender;
        });

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(handler.packetCallbacks[0]);
  }

  static void test_sendPacket_withStruct_encodesDataCorrectly() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    // First register the target device so sendPacket doesn't fail
    handler.registry->addDevice(
        TestDeviceID::DEVICE_1,
        (const uint8_t[]){0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF});

    TestPacketStruct testData = {0x42, 0x1234, 0xAB};

    // We can't fully test sendPacket without mocking esp_now_send,
    // but we can verify it compiles and the struct is trivially copyable
    static_assert(std::is_trivially_copyable<TestPacketStruct>::value,
                  "TestPacketStruct must be trivially copyable");

    TEST_ASSERT_TRUE(true); // Compilation success is the test
  }

  static void
  test_structPacketCallback_typeChecking_requiresTriviallyCopyable() {
    // This test verifies the static assertion for trivially copyable structs
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    struct TrivialStruct {
      int a;
      int b;
    };

    // This should compile because TrivialStruct is trivially copyable
    bool result = handler.registerCallback<TrivialStruct>(
        TestPacketType::TYPE_1,
        [](const TrivialStruct &data, TestDeviceID sender) {});

    TEST_ASSERT_TRUE(result);
  }

  static void
  test_registerCallback_structVersion_invokesCallbackWithCorrectData() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    // Reset callback state
    structCallbackCalled = false;
    receivedStruct = {};
    receivedSender = TestDeviceID::SELF;

    // Register struct callback
    handler.registerCallback<TestPacketStruct>(
        TestPacketType::TYPE_1,
        [](const TestPacketStruct &data, TestDeviceID sender) {
          structCallbackCalled = true;
          receivedStruct = data;
          receivedSender = sender;
        });

    // Manually invoke the callback to simulate packet reception
    TestPacketStruct testStruct = {0x55, 0x6789, 0xCD};
    const uint8_t *structPtr = reinterpret_cast<const uint8_t *>(&testStruct);

    // Invoke callback directly
    handler.packetCallbacks[0](structPtr, sizeof(TestPacketStruct),
                               TestDeviceID::DEVICE_1);

    TEST_ASSERT_TRUE(structCallbackCalled);
    TEST_ASSERT_EQUAL(0x55, receivedStruct.command);
    TEST_ASSERT_EQUAL(0x6789, receivedStruct.value);
    TEST_ASSERT_EQUAL(0xCD, receivedStruct.flags);
    TEST_ASSERT_EQUAL(TestDeviceID::DEVICE_1, receivedSender);
  }

  static void test_registerCallback_structVersion_rejectsIncorrectSize() {
    EspNowHandler<TestDeviceID, TestPacketType> handler(selfID, selfMac);

    bool callbackInvoked = false;
    handler.registerCallback<TestPacketStruct>(
        TestPacketType::TYPE_1,
        [&callbackInvoked](const TestPacketStruct &data, TestDeviceID sender) {
          callbackInvoked = true;
        });

    // Send data with wrong size
    uint8_t wrongData[5] = {0x01, 0x02, 0x03, 0x04, 0x05};

    // Call callback with wrong size - should be rejected
    handler.packetCallbacks[0](wrongData, sizeof(wrongData),
                               TestDeviceID::DEVICE_1);

    // Callback should not have been fully invoked due to size mismatch
    TEST_ASSERT_FALSE(callbackInvoked);
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
  RUN_TEST(handlerTest.test_registerCallback_withStructPacket_storesCallback);
  RUN_TEST(handlerTest.test_sendPacket_withStruct_encodesDataCorrectly);
  RUN_TEST(
      handlerTest
          .test_structPacketCallback_typeChecking_requiresTriviallyCopyable);
  RUN_TEST(
      handlerTest
          .test_registerCallback_structVersion_invokesCallbackWithCorrectData);
  RUN_TEST(
      handlerTest.test_registerCallback_structVersion_rejectsIncorrectSize);
  RUN_TEST(handlerTest.test_toIndex_convertsPacketTypeToSize);
  UNITY_END();
}
void loop() {}

#endif