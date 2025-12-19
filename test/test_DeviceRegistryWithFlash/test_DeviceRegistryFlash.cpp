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

void test_USE_FLASH_defined(void)
{
#ifdef USE_FLASH
    TEST_ASSERT_TRUE(USE_FLASH);
#else
    TEST_ASSERT_FALSE(USE_FLASH);
#endif
}

void test_getIdFromMac_with_pointer_found(void)
{
    uint8_t testMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    registry->setMac(5, testMac);
    registry->saveToFlash();
    
    DeviceRegistry newRegistry = DeviceRegistry();
    uint8_t id = newRegistry.getIdFromMac(testMac);
    TEST_ASSERT_EQUAL_UINT8(5, id);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_USE_FLASH_defined);
    RUN_TEST(test_getIdFromMac_with_pointer_found);
    UNITY_END();
}

void loop()
{
}