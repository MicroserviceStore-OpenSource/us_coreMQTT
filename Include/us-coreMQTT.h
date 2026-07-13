/*
 * @file
 *
 * @brief Public API interface for the coreMQTT Microservice.
 *
 * This Microservice wraps the FreeRTOS coreMQTT library (v04845c6) and exposes
 * a session-based MQTT client. The underlying TLS transport is provided by the
 * embedded Microservice Runtime (eMR) as a Universal Resource.
 *
 * IMPORTANT USAGE NOTES / LIMITATIONS:
 *   - Only ONE session is allowed at a time. A caller must acquire the session
 *     using us_coreMQTT_Connect() and release it using us_coreMQTT_Disconnect().
 *     A different caller can NOT use the session until it is released.
 *   - Because Microservices pass data by value (no references between
 *     executions) and the request/response package is limited to 252 bytes
 *     (244 bytes of payload), the following buffer limitations apply:
 *        - Maximum topic length        : US_COREMQTT_MAX_TOPIC_LEN  (64 bytes)
 *        - Maximum payload length      : US_COREMQTT_MAX_PAYLOAD_LEN(128 bytes)
 *        - Maximum URL/host length     : US_COREMQTT_MAX_URL_LEN    (64 bytes)
 *        - Maximum client id length    : US_COREMQTT_MAX_CLIENTID_LEN(48 bytes)
 *   - The TLS "Universal Resource" MUST be assigned to this Microservice.
 *   - The caller must assign the three Root Parameter Tags (root certificate,
 *     device certificate and private key) to the Microservice; these tags are
 *     passed to us_coreMQTT_Connect().
 *
 ******************************************************************************/

#ifndef US_COREMQTT_H
#define US_COREMQTT_H

#include "uService.h"

/*
 * Default Status
 */
typedef enum
{
    usStatus_Success = 0,
    /* Operation not defined or the access not granted */
    usStatus_InvalidOperation,
    /* Timeout occurred during the opereration */
    usStatus_Timeout,
    /* Microservice does not have any available session */
    usStatus_NoSessionSlotAvailable,
    /* Request to an invalid session */
    usStatus_InvalidSession,
    /* Invalid Parameter - Insufficient Input or Output Size  */
    usStatus_InvalidParam_UnsufficientSize,
    /* Invalid Parameter - Input or Output exceeds the allowed capacity  */
    usStatus_InvalidParam_SizeExceedAllowed,
    /* The developer can defines custom statuses */
    usStatus_CustomStart = 32
} usStatus;

/******************************* BUFFER LIMITS ********************************/
#define US_COREMQTT_MAX_TOPIC_LEN       (64U)
#define US_COREMQTT_MAX_PAYLOAD_LEN     (128U)
#define US_COREMQTT_MAX_URL_LEN         (64U)
#define US_COREMQTT_MAX_CLIENTID_LEN    (48U)

/****************************** TYPE DEFINITIONS ******************************/

/*
 * MQTT QoS levels (mirror of the open-source MQTTQoS_t values).
 */
typedef enum
{
    us_MQTTQoS0 = 0,
    us_MQTTQoS1 = 1,
    us_MQTTQoS2 = 2
} us_MQTTQoS_t;

/****************************** PUBLIC API ************************************/

/*
 * Initialise the Microservice user library. Must be called before any other
 * API function.
 */
SysStatus us_coreMQTT_Initialise(void);

/*
 * Establish an MQTT connection over TLS.
 *
 * This acquires the single session. The three Root Parameter Tags identify the
 * TLS credentials (root certificate, device certificate and private key) that
 * were assigned to this Microservice by the caller/eMR.
 *
 * @param host              Broker host/URL (max US_COREMQTT_MAX_URL_LEN bytes)
 * @param hostLen           Length of the host string
 * @param port              Broker TCP port
 * @param clientId          MQTT client identifier (max US_COREMQTT_MAX_CLIENTID_LEN)
 * @param clientIdLen       Length of the client identifier
 * @param keepAliveSeconds  MQTT keep-alive interval in seconds
 * @param rootCertTag       Root Parameter Tag for the root certificate
 * @param deviceCertTag     Root Parameter Tag for the device certificate
 * @param privateKeyTag     Root Parameter Tag for the private key
 *
 * @retval usStatus_Success on success.
 */
usStatus us_coreMQTT_Connect(const char* host, uint16_t hostLen, uint16_t port,
                             const char* clientId, uint16_t clientIdLen,
                             uint16_t keepAliveSeconds,
                             uint32_t rootCertTag, uint32_t deviceCertTag,
                             uint32_t privateKeyTag);

/*
 * Publish a message to a topic on the connected session.
 *
 * @param topic       Topic string (max US_COREMQTT_MAX_TOPIC_LEN bytes)
 * @param topicLen    Length of the topic
 * @param payload     Message payload (max US_COREMQTT_MAX_PAYLOAD_LEN bytes)
 * @param payloadLen  Length of the payload
 * @param qos         Quality of service level
 *
 * @retval usStatus_Success on success.
 */
usStatus us_coreMQTT_Publish(const char* topic, uint16_t topicLen,
                             const uint8_t* payload, uint16_t payloadLen,
                             us_MQTTQoS_t qos);

/*
 * Subscribe to a topic on the connected session.
 *
 * @param topic     Topic filter (max US_COREMQTT_MAX_TOPIC_LEN bytes)
 * @param topicLen  Length of the topic filter
 * @param qos       Quality of service level
 *
 * @retval usStatus_Success on success.
 */
usStatus us_coreMQTT_Subscribe(const char* topic, uint16_t topicLen,
                               us_MQTTQoS_t qos);

/*
 * Run the MQTT event loop once, processing incoming/outgoing packets.
 *
 * @param timeoutMs  Time to block in the process loop in milliseconds
 *
 * @retval usStatus_Success on success.
 */
usStatus us_coreMQTT_ProcessLoop(uint32_t timeoutMs);

/*
 * Disconnect the MQTT session and release the single session slot.
 *
 * @retval usStatus_Success on success.
 */
usStatus us_coreMQTT_Disconnect(void);

#endif /* US_COREMQTT_H */