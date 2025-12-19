#include <unity.h>
#include <DeviceRegistry.h>
#include <DeviceRegistry.cpp>

DeviceRegistry *registry;
Preferences unitPrefs;

void setUp(void)
{
    unitPrefs.begin("dReg", false);
    unitPrefs.clear();
    unitPrefs.end();
    registry = new DeviceRegistry();
}

void tearDown(void)
{
    delete registry;
}

bool check_FLASH_empty()
{
    unitPrefs.begin("dReg", false);
    size_t len = unitPrefs.getBytesLength("val");
    unitPrefs.end();
    delay(100); // Ensure flash operations complete
    return len == 0;
}

bool check_FLASH_not_empty()
{
    unitPrefs.begin("dReg", false);
    size_t len = unitPrefs.getBytesLength("val");
    unitPrefs.end();
    delay(100); // Ensure flash operations complete
    return len > 0;
}

void test_USE_FLASH_true(void)
{
#if USE_FLASH
    TEST_PASS();
#else
    TEST_FAIL_MESSAGE("USE_FLASH is not true");
#endif
}

void test_Registry_flash_initially_empty(void)
{
    TEST_ASSERT_TRUE(check_FLASH_empty());
}

void test_Registry_flash_not_empty_after_save(void)
{
    uint8_t testMac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};
    registry->setMac(1, testMac);
    registry->saveToFlash();

    TEST_ASSERT_TRUE(check_FLASH_not_empty());
}

void test_Registry_still_empty_after_test(void)
{
    TEST_ASSERT_TRUE(check_FLASH_empty());
}

void test_getIdFromMac_with_pointer_found(void)
{
    uint8_t testMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    registry->setMac(5, testMac);
    registry->saveToFlash();

    TEST_ASSERT_TRUE(check_FLASH_not_empty());
    
    DeviceRegistry newRegistry = DeviceRegistry();
    uint8_t id = newRegistry.getIdFromMac(testMac);
    boolean found = id != (uint8_t)RegistryStatus::ERROR_MAC_NOT_FOUND;
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_EQUAL_UINT8(5, id);
}

void setup()
{
    delay(2000); // Wait for serial monitor
    UNITY_BEGIN();
    RUN_TEST(test_USE_FLASH_true);
    RUN_TEST(test_Registry_flash_initially_empty);
    RUN_TEST(test_Registry_flash_not_empty_after_save);
    RUN_TEST(test_Registry_still_empty_after_test);
    RUN_TEST(test_getIdFromMac_with_pointer_found);
    UNITY_END();
}

void loop()
{
}