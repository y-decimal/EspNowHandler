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

void test_USE_FLASH_defined(void)
{
#ifdef USE_FLASH
    TEST_ASSERT_FALSE(USE_FLASH);
#else
    TEST_ASSERT_TRUE(USE_FLASH);
#endif
}

void test_getIdFromMac_with_pointer_found(void)
{
    uint8_t testMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    registry.setMac(5, testMac);

    uint8_t id = registry.getIdFromMac(testMac);
    TEST_ASSERT_EQUAL_UINT8(5, id);
}

void test_getIdFromMac_with_pointer_not_found(void)
{
    uint8_t testMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    uint8_t id = registry.getIdFromMac(testMac);
    TEST_ASSERT_EQUAL_UINT8(0xFF, id);
}

void test_getIdFromMac_with_array_found(void)
{
    std::array<uint8_t, 6> testMac = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    registry.setMac(10, testMac);

    uint8_t id = registry.getIdFromMac(testMac);
    TEST_ASSERT_EQUAL_UINT8(10, id);
}

void test_getIdFromMac_with_array_not_found(void)
{
    std::array<uint8_t, 6> testMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    uint8_t id = registry.getIdFromMac(testMac);
    TEST_ASSERT_EQUAL_UINT8(0xFF, id);
}

void test_getIdFromMac_multiple_devices(void)
{
    uint8_t mac1[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t mac2[6] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

    registry.setMac(2, mac1);
    registry.setMac(7, mac2);

    TEST_ASSERT_EQUAL_UINT8(2, registry.getIdFromMac(mac1));
    TEST_ASSERT_EQUAL_UINT8(7, registry.getIdFromMac(mac2));
}

void setup()
{
    delay(2000); // Wait for serial monitor
    UNITY_BEGIN();
    RUN_TEST(test_USE_FLASH_defined);
    RUN_TEST(test_getIdFromMac_with_pointer_found);
    RUN_TEST(test_getIdFromMac_with_pointer_not_found);
    RUN_TEST(test_getIdFromMac_with_array_found);
    RUN_TEST(test_getIdFromMac_with_array_not_found);
    RUN_TEST(test_getIdFromMac_multiple_devices);
    UNITY_END();
}

void loop()
{
    // Empty loop
}