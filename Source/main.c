#define LOG_ERROR_ENABLED   1
#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED    1
#include "uService.h"
#include "SysCall_UniversalResources.h"

#include "us-coreMQTT.h"

#include "us_Internal.h"

#ifndef CFG_US_MAX_NUM_OF_SESSION
#define CFG_US_MAX_NUM_OF_SESSION   1       /* Let us allow one session at a time */
#endif

/*
 * Network buffer used by the coreMQTT library. Sized to comfortably hold the
 * limited MQTT packets exchanged by this Microservice.
 */
#define US_COREMQTT_NETWORK_BUFFER_SIZE     (512U)

/*
 * Invalid sender identifier marker.
 */
#define US_COREMQTT_INVALID_SENDER          (0xFFU)

typedef struct
{
    struct
    {
        uint32_t inUse : 1;
    } flags;

    uint8_t     ownerSenderID;
    int32_t     tlsSocket;

    MQTTContext_t           mqttContext;
    TransportInterface_t    transport;
    MQTTFixedBuffer_t       fixedBuffer;
    NetworkContext_t*       networkContext;

    uint8_t     networkBuffer[US_COREMQTT_NETWORK_BUFFER_SIZE];
} usSessionContext;

/*
 * The coreMQTT NetworkContext_t is an opaque type defined by this Microservice.
 * It carries the TLS socket handle.
 */
struct NetworkContext
{
    int32_t tlsSocket;
};

PRIVATE void startService(void);
PRIVATE void processRequest(uint8_t senderID, usRequestPackage* request);
PRIVATE void sendError(uint8_t receiverID, uint16_t operation, uint8_t status);

PRIVATE int32_t transportSend(NetworkContext_t* pNetworkContext, const void* pBuffer, size_t bytesToSend);
PRIVATE int32_t transportRecv(NetworkContext_t* pNetworkContext, void* pBuffer, size_t bytesToRecv);
PRIVATE uint32_t getTimeMs(void);
PRIVATE bool eventCallback(struct MQTTContext * pContext,
                           struct MQTTPacketInfo * pPacketInfo,
                           struct MQTTDeserializedInfo * pDeserializedInfo,
                           enum MQTTSuccessFailReasonCode * pReasonCode,
                           struct MQTTPropBuilder * pSendPropsBuffer,
                           struct MQTTPropBuilder * pGetPropsBuffer);

PRIVATE usSessionContext session;
PRIVATE NetworkContext_t sessionNetworkContext;

PRIVATE uint32_t getTimeMs(void)
{
    return (uint32_t)Sys_GetTimeInMs();
}

int main()
{
    SysStatus retVal;

    uService_PrintIntro();

    SYS_INITIALISE_IPC_MESSAGEBOX(retVal, CFG_US_MAX_NUM_OF_SESSION);
    if (retVal != SysStatus_Success)
    {
        LOG_ERROR("Failed to initialise MessageBox. Error : %d", retVal);
    }
    else
    {
        startService();
    }

    LOG_ERROR("Exiting the Microservice...");
    Sys_Exit();
}


PRIVATE int32_t transportSend(NetworkContext_t* pNetworkContext, const void* pBuffer, size_t bytesToSend)
{
    uint32_t writtenLen = 0;
    int32_t status = 0;
    SysStatus retVal;

    if (pNetworkContext == NULL)
    {
        return -1;
    }

    retVal = tls_write(pNetworkContext->tlsSocket, (const uint8_t*)pBuffer, (uint32_t)bytesToSend, &writtenLen, &status);
    if (retVal != SysStatus_Success)
    {
        return -1;
    }

    return (int32_t)writtenLen;
}

PRIVATE int32_t transportRecv(NetworkContext_t* pNetworkContext, void* pBuffer, size_t bytesToRecv)
{
    uint32_t readLen = 0;
    int32_t status = 0;
    SysStatus retVal;

    if (pNetworkContext == NULL)
    {
        return -1;
    }

    retVal = tls_read(pNetworkContext->tlsSocket, (uint8_t*)pBuffer, (uint32_t)bytesToRecv, &readLen, &status);
    if (retVal != SysStatus_Success)
    {
        return -1;
    }

    return (int32_t)readLen;
}

PRIVATE bool eventCallback(struct MQTTContext * pContext,
                            struct MQTTPacketInfo * pPacketInfo,
                            struct MQTTDeserializedInfo * pDeserializedInfo,
                            enum MQTTSuccessFailReasonCode * pReasonCode,
                            struct MQTTPropBuilder * pSendPropsBuffer,
                            struct MQTTPropBuilder * pGetPropsBuffer)
{
    (void)pContext;
    (void)pPacketInfo;
    (void)pDeserializedInfo;
    (void)pReasonCode;
    (void)pSendPropsBuffer;
    (void)pGetPropsBuffer;

    /* Incoming publishes / acks are handled inside the library; nothing to do here. */

    return true;
}

PRIVATE void startService(void)
{
    usRequestPackage request;

    uint32_t sequenceNo; (void)sequenceNo;
    usStatus responseStatus;
    uint8_t senderID = 0xFF;

    while (true)
    {
        bool dataReceived = false;
        uint32_t receivedLen = 0;
        responseStatus = usStatus_Success;

        (void)Sys_IsMessageReceived(&dataReceived, &receivedLen, &sequenceNo);
        if (!dataReceived || receivedLen == 0)
        {
            /* Sleep until receive an IPC message */
            Sys_WaitForEvent(SysEvent_IPCMessage, 0);

            continue;
        }

        if (receivedLen <= USERVICE_PACKAGE_HEADER_SIZE)
        {
            responseStatus = usStatus_InvalidParam_UnsufficientSize;
            LOG_PRINTF(" > Unsufficint Mandatory Received Length (%d)/(%d)",
                receivedLen, USERVICE_PACKAGE_HEADER_SIZE);
        }

        /* Get the message */
        (void)Sys_ReceiveMessage(&senderID, (uint8_t*)&request, receivedLen, &sequenceNo);

        /* Do not process the message if there was an error */
        if (responseStatus != usStatus_Success)
        {
            sendError(senderID, request.header.operation, responseStatus);
            continue;
        }

        /* Process the request */
        processRequest(senderID, &request);
    }
}

PRIVATE void processRequest(uint8_t senderID, usRequestPackage* request)
{
    SysStatus retVal = SysStatus_Success;
    usResponsePackage response;
    uint32_t sequenceNo;

    (void)retVal;

    response.header = request->header;

    /*
     * Session ownership check for all operations except Connect.
     * Connect acquires the session, everything else requires the caller to be
     * the current owner.
     */
    if (request->header.operation != usOp_Connect)
    {
        if (!session.flags.inUse)
        {
            sendError(senderID, request->header.operation, usStatus_InvalidSession);
            return;
        }
        if (session.ownerSenderID != senderID)
        {
            sendError(senderID, request->header.operation, usStatus_InvalidOperation);
            return;
        }
    }

    switch (request->header.operation)
    {
        case usOp_Connect:
        {
            int32_t tlsStatus = 0;
            MQTTStatus_t mqttStatus;
            MQTTConnectInfo_t connectInfo;
            bool sessionPresent = false;
            SysUniversalResourceCredentials rootCert;
            SysUniversalResourceCredentials deviceCert;
            SysUniversalResourceCredentials privateKey;

            if (session.flags.inUse)
            {
                sendError(senderID, request->header.operation, usStatus_NoSessionSlotAvailable);
                return;
            }

            /* Get a TLS socket */
            if (tls_getsocket(&session.tlsSocket, &tlsStatus) != SysStatus_Success)
            {
                LOG_ERROR("tls_getsocket failed. status : %d", tlsStatus);
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            /* Prepare TLS credentials from Root Parameter Tags */
            rootCert.flags.type = SYS_UNIVERSAL_RES_CREDENTIAL_TYPE_ROOTPARAM;
            rootCert.context.rootParam.tag = request->payload.connect.rootCertTag;
            deviceCert.flags.type = SYS_UNIVERSAL_RES_CREDENTIAL_TYPE_ROOTPARAM;
            deviceCert.context.rootParam.tag = request->payload.connect.deviceCertTag;
            privateKey.flags.type = SYS_UNIVERSAL_RES_CREDENTIAL_TYPE_ROOTPARAM;
            privateKey.context.rootParam.tag = request->payload.connect.privateKeyTag;

            if (tls_connect(session.tlsSocket, &rootCert, &deviceCert, &privateKey,
                            request->payload.connect.host, request->payload.connect.hostLen,
                            request->payload.connect.port, 10000, &tlsStatus) != SysStatus_Success)
            {
                LOG_ERROR("tls_connect failed. status : %d", tlsStatus);
                (void)tls_close(session.tlsSocket, &tlsStatus);
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            /* Setup the network context and transport interface */
            sessionNetworkContext.tlsSocket = session.tlsSocket;
            session.networkContext = &sessionNetworkContext;

            session.transport.pNetworkContext = session.networkContext;
            session.transport.send = transportSend;
            session.transport.recv = transportRecv;

            session.fixedBuffer.pBuffer = session.networkBuffer;
            session.fixedBuffer.size = US_COREMQTT_NETWORK_BUFFER_SIZE;

            mqttStatus = MQTT_Init(&session.mqttContext, &session.transport,
                                   getTimeMs, eventCallback, &session.fixedBuffer);
            if (mqttStatus != MQTTSuccess)
            {
                LOG_ERROR("MQTT_Init failed. status : %d", mqttStatus);
                (void)tls_close(session.tlsSocket, &tlsStatus);
                response.payload.connect.mqttStatus = (int32_t)mqttStatus;
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            connectInfo.cleanSession = true;
            connectInfo.pClientIdentifier = request->payload.connect.clientId;
            connectInfo.clientIdentifierLength = request->payload.connect.clientIdLen;
            connectInfo.keepAliveSeconds = request->payload.connect.keepAliveSeconds;
            connectInfo.pUserName = NULL;
            connectInfo.userNameLength = 0;
            connectInfo.pPassword = NULL;
            connectInfo.passwordLength = 0;

            mqttStatus = MQTT_Connect(&session.mqttContext, &connectInfo, NULL, 5000U, &sessionPresent, NULL, NULL);
            if (mqttStatus != MQTTSuccess)
            {
                LOG_ERROR("MQTT_Connect failed. status : %d", mqttStatus);
                (void)tls_close(session.tlsSocket, &tlsStatus);
                response.payload.connect.mqttStatus = (int32_t)mqttStatus;
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            session.flags.inUse = 1;
            session.ownerSenderID = senderID;

            response.payload.connect.mqttStatus = (int32_t)mqttStatus;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_Publish:
        {
            MQTTStatus_t mqttStatus;
            MQTTPublishInfo_t publishInfo;
            uint16_t packetId;

            publishInfo.qos = (MQTTQoS_t)request->payload.publish.qos;
            publishInfo.retain = false;
            publishInfo.dup = false;
            publishInfo.pTopicName = request->payload.publish.topic;
            publishInfo.topicNameLength = request->payload.publish.topicLen;
            publishInfo.pPayload = request->payload.publish.payload;
            publishInfo.payloadLength = request->payload.publish.payloadLen;

            packetId = MQTT_GetPacketId(&session.mqttContext);

            mqttStatus = MQTT_Publish(&session.mqttContext, &publishInfo, packetId, NULL);
            if (mqttStatus != MQTTSuccess)
            {
                LOG_ERROR("MQTT_Publish failed. status : %d", mqttStatus);
                response.payload.publish.mqttStatus = (int32_t)mqttStatus;
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            response.payload.publish.mqttStatus = (int32_t)mqttStatus;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_Subscribe:
        {
            MQTTStatus_t mqttStatus;
            MQTTSubscribeInfo_t subscribeInfo;
            uint16_t packetId;

            subscribeInfo.qos = (MQTTQoS_t)request->payload.subscribe.qos;
            subscribeInfo.pTopicFilter = request->payload.subscribe.topic;
            subscribeInfo.topicFilterLength = request->payload.subscribe.topicLen;
            subscribeInfo.noLocalOption = false;
            subscribeInfo.retainAsPublishedOption = false;
            subscribeInfo.retainHandlingOption = retainSendOnSub; /* or whatever your intended default is */

            packetId = MQTT_GetPacketId(&session.mqttContext);

            mqttStatus = MQTT_Subscribe(&session.mqttContext, &subscribeInfo, 1U, packetId, NULL);
            if (mqttStatus != MQTTSuccess)
            {
                LOG_ERROR("MQTT_Subscribe failed. status : %d", mqttStatus);
                response.payload.subscribe.mqttStatus = (int32_t)mqttStatus;
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            response.payload.subscribe.mqttStatus = (int32_t)mqttStatus;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_ProcessLoop:
        {
            MQTTStatus_t mqttStatus;

            (void)tls_periodic();

            mqttStatus = MQTT_ProcessLoop(&session.mqttContext);
            if ((mqttStatus != MQTTSuccess) && (mqttStatus != MQTTNeedMoreBytes))
            {
                LOG_ERROR("MQTT_ProcessLoop failed. status : %d", mqttStatus);
                response.payload.processLoop.mqttStatus = (int32_t)mqttStatus;
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            response.payload.processLoop.mqttStatus = (int32_t)mqttStatus;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_Disconnect:
        {
            MQTTStatus_t mqttStatus;
            int32_t tlsStatus = 0;

            mqttStatus = MQTT_Disconnect(&session.mqttContext, NULL, NULL);
            if (mqttStatus != MQTTSuccess)
            {
                LOG_WARNING("MQTT_Disconnect returned status : %d", mqttStatus);
            }

            (void)tls_close(session.tlsSocket, &tlsStatus);

            session.flags.inUse = 0;
            session.ownerSenderID = US_COREMQTT_INVALID_SENDER;

            response.payload.disconnect.mqttStatus = (int32_t)mqttStatus;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        /* Unrecognised operation */
        default:
            sendError(senderID, response.header.operation, usStatus_InvalidOperation);
            break;
    }
}

PRIVATE void sendError(uint8_t receiverID, uint16_t operation, uint8_t status)
{
    uint32_t sequenceNo;
    (void)sequenceNo;
    usResponsePackage response =
    {
        .header.operation = operation,
        .header.status = status,
        .header.length = 0
    };

    (void)Sys_SendMessage(receiverID, (uint8_t*)&response, sizeof(response), &sequenceNo);
}