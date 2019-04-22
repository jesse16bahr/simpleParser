/* MessageHandler.h
 *
 * This defines a class that may be used to parse, store and print
 *   packets of data for a coding exercise given by IMSAR.
 *
 * Copyright 2018 Jesse Bahr
 * All rights reserved.
 */

#ifndef MessageHandler_h
#define MessageHandler_h

#include <cJSON.h>
#include <stdint.h>
#include <stdlib.h>



#define MESSAGE_HANDLER_COMMAND_SETSARMODE      0xFF03
#define MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE 0xFF05
#define MESSAGE_HANDLER_COMMAND_HEARTBEAT       0xFF08

/*
 * @brief field indeces and size enumerations
 */
enum
{
    fieldIndex_keySignature = 0,
    fieldSize_keySignature  = 2,

    fieldIndex_headerChecksum = 2,
    fieldSize_headerChecksum  = 2,

    fieldIndex_dataChecksum  = 4,
    fieldSize_dataChecksum   = 2,

    fieldIndex_messageProperties = 6,
    fieldSize_messageProperties  = 2,

    fieldIndex_commandCode = 8,
    fieldSize_commandCode  = 2,

    fieldIndex_payloadSize = 10,
    fieldSize_payloadSize  = 2,

    fieldIndex_payload     = 12,
};

enum
{
    messageHandler_prefixSize = 6,
    messageHandler_headerSize = 6,
};

enum
{
    parseBufferSize = 1024,
};


#pragma pack(push, 1)
typedef union 
{
    struct
    {
        uint16_t priority       : 4;
        uint16_t ackDesignation : 2;
        uint16_t version        : 8;
        uint16_t reserved       : 2;
    };
    uint16_t value;
} MessageHandler_MessageProperties;
#pragma pack(pop)

typedef union
{
    #pragma pack(push, 1)
    struct
    {
        MessageHandler_MessageProperties properties;
        uint16_t                         commandCode;
        uint16_t                         payloadLength;
    };
    #pragma pack(pop)
    uint8_t headerBytes[messageHandler_headerSize];
} MessageHandler_Header;

#pragma pack(push, 1)
typedef struct
{
    uint32_t epochTime_seconds;
    uint32_t serialNumber;
    int16_t  voltage_cV;
    int8_t   temperature_C;
    uint8_t  mode;
} MessageHandler_HeartbeatPayload;
#pragma pack(pop)

typedef union
{
    cJSON*                          json;
    bool                            enableStandby;
    MessageHandler_HeartbeatPayload heartbeat;
} MessageHandler_Payload;



class MessageHandler
{
    public:

        MessageHandler();
        ~MessageHandler();

        MessageHandler(uint8_t* rawBuffer, uint32_t size);

        /*
         * @brief Outputs the message in human-readable format to stdout
         */
        void print(void);

        /*
         * @brief: Parse a single byte as part of a stream of bytes
         *
         * @param[out] byte - pointer to array to add to parseBuffer
         * @return     full message compiled
         */
        bool parseByte(char byte);

        /*
         * @brief: Parse bytes as they come in, handles multiple calls
         *
         * @param[out] buffer - pointer to array to add to parseBuffer
         * @return     full message compiled
         */
        bool parseBytes(uint8_t* buffer, uint32_t size, uint8_t** remainingBuffer);

        /*
         * @brief: Serialize a message built by originally
         *
         * @param[out] bufferPtr - this is a double pointer to be able return a new pointer to the raw
         * @return     size      - serialization size
         */
        uint32_t getSerialized(uint8_t** buffer);

        /*
         * @brief Set the hearbeat member of the message
         *        This will also set the message type to heartbeat.
         * 
         * @param epochSeconds  - epoch time in seconds
         * @param serialNumber  - serial number of device
         * @param votage_V      - voltage measured in hundredths of a volt (centivolts)
         * @param temperature_C - 
         * @param mode          -
         */
        void setHeartbeat(MessageHandler_HeartbeatPayload* heartbeat);

        /*
         * @brief Get the hearbeat member data fields of the message
         * 
         * @param[out] epochSeconds  - epoch time in seconds
         * @param[out] serialNumber  - serial number of device
         * @param[out] votage_V      - voltage measured in hundredths of a volt (centivolts)
         * @param[out] temperature_C - 
         * @param[out] mode          -
         */
        void getHeartbeat(MessageHandler_HeartbeatPayload* heartbeat);

        /*
         * @brief Set the message type to JSON string and set the data with the given string
         *        This does not validate that the json string is valid JSON
         *        TBD: Validate the JSON string or use a json structure like cJSON here instead
         *        TBD: Split JSON into seperate fields and set those instead
         *
         * @param jsonString - JSON string
         */
        bool setPayloadJson(char* jsonString);

        /*
         * @brief retrieve a pointer the JSON payload of the message
         *
         * @return JSON string pointer
         */
        char* getPayloadJsonString(void);

        /*
         * @brief set the message type to enable/disable standby state
         *
         * @param enable - value, to which to set the standby enabled field of the message
         */
        void setPayloadStandbyEnabled(bool enable);

        /*
         * @brief retrieve the current value of the standby enable field for the message
         *
         * @return statndyEnabled
         */
        bool getPayloadStandbyEnabled(void);

        /*
         * @brief Get the length of the data section of the message
         *
         * @return message data(payload) length
         */
        uint16_t getPayloadLength(void);

        /*
         * @brief Retrieve the command code that specifies the message data type
         *
         * @return commandCode
         */
        uint16_t getCommandCode(void);

        /*
         * @brief Set the message priority, ack/nack designation and version using the structure
         *
         * @param properties - a structure that seperates 16 bit value into needed bit fields
         */
        void setMessageProperties(MessageHandler_MessageProperties* properties);

        /*
         * @brief Retrieve the message properties fields
         *
         * @param[out] a structure that seperates 16 bit value into needed bit fields
         */
        void getMessageProperties(MessageHandler_MessageProperties* properties);

    private:
        uint8_t               parseBuffer[parseBufferSize];
        uint32_t              parseIndex;

        uint8_t*              serializedMessage;
        uint32_t              serializedSize;

        /*
         * @brief These are all of the fields that make up a message
         */
        const char*           packetSignature = "TT";
        uint16_t              headerChecksum;
        uint16_t              payloadChecksum;

        MessageHandler_Header header;
        MessageHandler_Payload payload;
};


#endif // MessageHandler_h