#ifndef __US_INTERNAL_H
#define __US_INTERNAL_H

#include "uService.h"

#include "us-coreMQTT.h"

#include "coreMQTT/source/include/core_mqtt.h"
#include "coreMQTT/source/include/core_mqtt_state.h"

/*
 * Operations supported by this Microservice.
 */
typedef enum
{
    usOp_Connect = 0,
    usOp_Publish,
    usOp_Subscribe,
    usOp_ProcessLoop,
    usOp_Disconnect
} usOperations;

/*
 * Request Package
 */
typedef struct
{
    uServicePackageHeader header;

    union
    {
        struct
        {
            char     host[US_COREMQTT_MAX_URL_LEN];
            char     clientId[US_COREMQTT_MAX_CLIENTID_LEN];
            uint16_t hostLen;
            uint16_t clientIdLen;
            uint16_t port;
            uint16_t keepAliveSeconds;
            uint32_t rootCertTag;
            uint32_t deviceCertTag;
            uint32_t privateKeyTag;
        } connect;

        struct
        {
            char     topic[US_COREMQTT_MAX_TOPIC_LEN];
            uint8_t  payload[US_COREMQTT_MAX_PAYLOAD_LEN];
            uint16_t topicLen;
            uint16_t payloadLen;
            us_MQTTQoS_t qos;
        } publish;

        struct
        {
            char     topic[US_COREMQTT_MAX_TOPIC_LEN];
            uint16_t topicLen;
            us_MQTTQoS_t qos;
        } subscribe;

        struct
        {
            uint32_t timeoutMs;
        } processLoop;
    } payload;
} usRequestPackage;

/*
 * Response Package
 */
typedef struct
{
    uServicePackageHeader header;

    union
    {
        struct
        {
            int32_t mqttStatus;
        } connect;

        struct
        {
            int32_t mqttStatus;
        } publish;

        struct
        {
            int32_t mqttStatus;
        } subscribe;

        struct
        {
            int32_t mqttStatus;
        } processLoop;

        struct
        {
            int32_t mqttStatus;
        } disconnect;
    } payload;
} usResponsePackage;

#endif /* __US_INTERNAL_H */