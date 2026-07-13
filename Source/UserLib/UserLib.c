/*
 * @file
 *
 * @brief Microservice API static library source file. This source file/library
 *        runs in the caller execution.
 *
 ******************************************************************************/

/********************************* INCLUDES ***********************************/

#include "us-coreMQTT.h"

#include "us_Internal.h"

#include "uService.h"

#include <string.h>

/***************************** MACRO DEFINITIONS ******************************/

/***************************** TYPE DEFINITIONS *******************************/
typedef struct
{
    struct
    {
        uint32_t initialised        : 1;
    } flags;

    /*
    * The "Execution Index" is a system wide enumaration by the Microservice Runtime
    * to interact with the Microservice.
    */
    uint32_t execIndex;
} uS_UserLibSettings;

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/
PRIVATE uS_UserLibSettings userLibSettings;

PRIVATE const char usUID[SYS_EXEC_NAME_MAX_LENGTH] = "COREMQTT";

/***************************** PRIVATE FUNCTIONS *******************************/

/***************************** PUBLIC FUNCTIONS *******************************/
#define INITIALISE_FUNCTIONEXPAND(a, b, c) a##b##c
#define INITIALISE_FUNCTION(name) INITIALISE_FUNCTIONEXPAND(us_, name, _Initialise)
SysStatus INITIALISE_FUNCTION(USERVICE_NAME_NONSTR)(void)
{
    /* Get the Microservice Index to interact with the Microservice */
    return uService_Initialise(usUID, &userLibSettings.execIndex);
}

usStatus us_coreMQTT_Connect(const char* host, uint16_t hostLen, uint16_t port,
                             const char* clientId, uint16_t clientIdLen,
                             uint16_t keepAliveSeconds,
                             uint32_t rootCertTag, uint32_t deviceCertTag,
                             uint32_t privateKeyTag)
{
    const uint32_t timeoutInMs = 60000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    if ((host == NULL) || (clientId == NULL))
    {
        return usStatus_InvalidParam_UnsufficientSize;
    }

    if ((hostLen > US_COREMQTT_MAX_URL_LEN) || (clientIdLen > US_COREMQTT_MAX_CLIENTID_LEN))
    {
        return usStatus_InvalidParam_SizeExceedAllowed;
    }

    {
        request.header.operation = usOp_Connect;
        request.header.length = sizeof(usRequestPackage);

        memcpy(request.payload.connect.host, host, hostLen);
        request.payload.connect.hostLen = hostLen;
        memcpy(request.payload.connect.clientId, clientId, clientIdLen);
        request.payload.connect.clientIdLen = clientIdLen;
        request.payload.connect.port = port;
        request.payload.connect.keepAliveSeconds = keepAliveSeconds;
        request.payload.connect.rootCertTag = rootCertTag;
        request.payload.connect.deviceCertTag = deviceCertTag;
        request.payload.connect.privateKeyTag = privateKeyTag;
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if ((retVal == SysStatus_Success) && (response.header.status == usStatus_Success))
    {
        return usStatus_Success;
    }

    return (usStatus)response.header.status;
}

usStatus us_coreMQTT_Publish(const char* topic, uint16_t topicLen,
                             const uint8_t* payload, uint16_t payloadLen,
                             us_MQTTQoS_t qos)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    if ((topic == NULL) || (payload == NULL))
    {
        return usStatus_InvalidParam_UnsufficientSize;
    }

    if ((topicLen > US_COREMQTT_MAX_TOPIC_LEN) || (payloadLen > US_COREMQTT_MAX_PAYLOAD_LEN))
    {
        return usStatus_InvalidParam_SizeExceedAllowed;
    }

    {
        request.header.operation = usOp_Publish;
        request.header.length = sizeof(usRequestPackage);

        memcpy(request.payload.publish.topic, topic, topicLen);
        request.payload.publish.topicLen = topicLen;
        memcpy(request.payload.publish.payload, payload, payloadLen);
        request.payload.publish.payloadLen = payloadLen;
        request.payload.publish.qos = qos;
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if ((retVal == SysStatus_Success) && (response.header.status == usStatus_Success))
    {
        return usStatus_Success;
    }

    return (usStatus)response.header.status;
}

usStatus us_coreMQTT_Subscribe(const char* topic, uint16_t topicLen,
                               us_MQTTQoS_t qos)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    if (topic == NULL)
    {
        return usStatus_InvalidParam_UnsufficientSize;
    }

    if (topicLen > US_COREMQTT_MAX_TOPIC_LEN)
    {
        return usStatus_InvalidParam_SizeExceedAllowed;
    }

    {
        request.header.operation = usOp_Subscribe;
        request.header.length = sizeof(usRequestPackage);

        memcpy(request.payload.subscribe.topic, topic, topicLen);
        request.payload.subscribe.topicLen = topicLen;
        request.payload.subscribe.qos = qos;
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if ((retVal == SysStatus_Success) && (response.header.status == usStatus_Success))
    {
        return usStatus_Success;
    }

    return (usStatus)response.header.status;
}

usStatus us_coreMQTT_ProcessLoop(uint32_t timeoutMs)
{
    const uint32_t timeoutInMs = 60000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    {
        request.header.operation = usOp_ProcessLoop;
        request.header.length = sizeof(usRequestPackage);

        request.payload.processLoop.timeoutMs = timeoutMs;
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if ((retVal == SysStatus_Success) && (response.header.status == usStatus_Success))
    {
        return usStatus_Success;
    }

    return (usStatus)response.header.status;
}

usStatus us_coreMQTT_Disconnect(void)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    {
        request.header.operation = usOp_Disconnect;
        request.header.length = sizeof(usRequestPackage);
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if ((retVal == SysStatus_Success) && (response.header.status == usStatus_Success))
    {
        return usStatus_Success;
    }

    return (usStatus)response.header.status;
}