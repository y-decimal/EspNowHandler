#include <unity.h>
#include <DeviceRegistry.h>
#include <DeviceRegistry.cpp>

DeviceRegistry registry = DeviceRegistry();

void setUp(void)
{
}

void tearDown(void)
{
}

void test_USE_FLASH_true(void)
{
#if USE_FLASH
    TEST_FAIL_MESSAGE("USE_FLASH is true");
#else
    TEST_PASS();
#endif
}

void test_getDeviceMac_found(void)
{
    uint8_t testMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    const char *deviceName = "TestDevice";
    registry.addDevice(deviceName, testMac);

    const uint8_t *returnedMac = registry.getDeviceMac(deviceName);
    TEST_ASSERT_NOT_NULL(returnedMac);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testMac, returnedMac, 6);
}

void test_getDeviceMac_not_found(void)
{
    uint8_t testMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    const uint8_t *returnedMac = registry.getDeviceMac("NonExistentDevice");
    TEST_ASSERT_NULL(returnedMac);
}

void test_getUpdateDeviceMac_found(void)
{
    uint8_t initialMac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t newMac[6] = {0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB};
    const char *deviceName = "UpdateDevice";

    registry.addDevice(deviceName, initialMac);
    bool updateResult = registry.updateDeviceMac(deviceName, newMac);
    TEST_ASSERT_TRUE(updateResult);

    const uint8_t *returnedMac = registry.getDeviceMac(deviceName);
    TEST_ASSERT_NOT_NULL(returnedMac);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(newMac, returnedMac, 6);
}

void test_getUpdateDeviceMac_not_found(void)
{
    uint8_t newMac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};
    const char *deviceName = "NonExistentDevice";

    bool updateResult = registry.updateDeviceMac(deviceName, newMac);
    TEST_ASSERT_FALSE(updateResult);
}

void test_getDeviceMac_multiple_devices(void)
{
    uint8_t mac1[6] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
    uint8_t mac2[6] = {0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0};
    const char *deviceName1 = "DeviceOne";
    const char *deviceName2 = "DeviceTwo";

    registry.addDevice(deviceName1, mac1);
    registry.addDevice(deviceName2, mac2);

    const uint8_t *returnedMac1 = registry.getDeviceMac(deviceName1);
    TEST_ASSERT_NOT_NULL(returnedMac1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mac1, returnedMac1, 6);

    const uint8_t *returnedMac2 = registry.getDeviceMac(deviceName2);
    TEST_ASSERT_NOT_NULL(returnedMac2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mac2, returnedMac2, 6);
}

void test_removeDevice_found(void)
{
    uint8_t testMac[6] = {0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56};
    const char *deviceName = "RemovableDevice";

    registry.addDevice(deviceName, testMac);
    bool removeResult = registry.removeDevice(deviceName);
    TEST_ASSERT_TRUE(removeResult);

    const uint8_t *returnedMac = registry.getDeviceMac(deviceName);
    TEST_ASSERT_NULL(returnedMac);
}

void test_removeDevice_not_found(void)
{
    const char *deviceName = "NonExistentDevice";

    bool removeResult = registry.removeDevice(deviceName);
    TEST_ASSERT_FALSE(removeResult);
}

void setup()
{
    delay(2000); // Wait for serial monitor
    UNITY_BEGIN();
    RUN_TEST(test_USE_FLASH_true);
    RUN_TEST(test_getDeviceMac_found);
    RUN_TEST(test_getDeviceMac_not_found);
    RUN_TEST(test_getUpdateDeviceMac_found);
    RUN_TEST(test_getUpdateDeviceMac_not_found);
    RUN_TEST(test_getDeviceMac_multiple_devices);
    RUN_TEST(test_removeDevice_found);
    RUN_TEST(test_removeDevice_not_found);
    UNITY_END();
}

void loop()
{
    // Empty loop
}