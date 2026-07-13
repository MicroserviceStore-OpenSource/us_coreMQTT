/* Enable all the log levels */
#define LOG_INFO_ENABLED        1
#define LOG_WARNING_ENABLED     1
#define LOG_ERROR_ENABLED       1
#include "SysCall.h"

#include "us-coreMQTT.h"

#include "VendorRPCustomTags.h"

#include <string.h>

int main(void)
{
    SysStatus retVal;
    bool testPass = true;

    LOG_PRINTF(" > Container : Microservice Test User App");

    SYS_INITIALISE_IPC_MESSAGEBOX(retVal, 4);

    retVal = us_coreMQTT_Initialise();
    if (retVal != SysStatus_Success)
    {
        if (retVal == SysStatus_NotFound)
        {
            LOG_PRINTF(" > us_coreMQTT does not exists.");
        }
        else
        {
            LOG_PRINTF(" > us_coreMQTT_Initialise failed with error %d. Exiting the User Application...", retVal);
        }
        Sys_Exit();
    }

    /* Test Cases */
    {
        usStatus expected;
        usStatus actual;

        const char* host        = "test.mosquitto.org";
        const char* clientId    = "usCoreMqttSmokeTest";
        const char* topic       = "us/coreMQTT/test";
        const char* payload     = "hello-from-microservice";

        /* Test 1 : Connect (smoke - expected to acquire the single session) */
        expected = usStatus_Success;
        actual = us_coreMQTT_Connect(host, (uint16_t)strlen(host), 8883,
                                     clientId, (uint16_t)strlen(clientId),
                                     60U,
                                     RP_CUSTOM_ITEM_ROOTCA, RP_CUSTOM_ITEM_DEVCERT, RP_CUSTOM_ITEM_PKEY);
        LOG_PRINTF(" > Test Connect    : expected %d, actual %d -> %s",
                   expected, actual, (actual == expected) ? "SUCCESS" : "FAIL");
        if (actual != expected)
        {
            testPass = false;
        }

        if (actual == usStatus_Success)
        {
            /* Test 2 : Subscribe */
            expected = usStatus_Success;
            actual = us_coreMQTT_Subscribe(topic, (uint16_t)strlen(topic), us_MQTTQoS0);
            LOG_PRINTF(" > Test Subscribe  : expected %d, actual %d -> %s",
                       expected, actual, (actual == expected) ? "SUCCESS" : "FAIL");
            if (actual != expected)
            {
                testPass = false;
            }

            /* Test 3 : Publish */
            expected = usStatus_Success;
            actual = us_coreMQTT_Publish(topic, (uint16_t)strlen(topic),
                                         (const uint8_t*)payload, (uint16_t)strlen(payload),
                                         us_MQTTQoS0);
            LOG_PRINTF(" > Test Publish    : expected %d, actual %d -> %s",
                       expected, actual, (actual == expected) ? "SUCCESS" : "FAIL");
            if (actual != expected)
            {
                testPass = false;
            }

            /* Test 4 : ProcessLoop */
            expected = usStatus_Success;
            actual = us_coreMQTT_ProcessLoop(1000U);
            LOG_PRINTF(" > Test ProcessLoop: expected %d, actual %d -> %s",
                       expected, actual, (actual == expected) ? "SUCCESS" : "FAIL");
            if (actual != expected)
            {
                testPass = false;
            }

            /* Test 5 : Disconnect (releases session) */
            expected = usStatus_Success;
            actual = us_coreMQTT_Disconnect();
            LOG_PRINTF(" > Test Disconnect : expected %d, actual %d -> %s",
                       expected, actual, (actual == expected) ? "SUCCESS" : "FAIL");
            if (actual != expected)
            {
                testPass = false;
            }
        }

        /* Test 6 : Operation on a non-existent session must be rejected */
        expected = usStatus_InvalidSession;
        actual = us_coreMQTT_Publish(topic, (uint16_t)strlen(topic),
                                     (const uint8_t*)payload, (uint16_t)strlen(payload),
                                     us_MQTTQoS0);
        LOG_PRINTF(" > Test NoSession  : expected %d, actual %d -> %s",
                   expected, actual, (actual == expected) ? "SUCCESS" : "FAIL");
        if (actual != expected)
        {
            testPass = false;
        }
    }

    LOG_PRINTF(" > usTest : %s", testPass ? "SUCCESS" : "FAIL");
    /* Exit the Container */
    Sys_Exit();

    return 0;
}