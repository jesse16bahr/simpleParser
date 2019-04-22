/* MessageHandler.h
 *
 * This defines a class that may be used to parse, store and print
 *   packets of data for a coding exercise given by IMSAR.
 * May also be used to create and serialize messages of this type
 *
 *
 * Copyright 2018 Jesse Bahr
 *  All rights reserved.
 */

#include "MessageHandler.h"
#include "cJSON.h"

#include <stdio.h>      /* printf */
#include <assert.h>     /* assert */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <iostream>

using namespace std;



static uint8_t* readLittle16(uint8_t* bytes, uint16_t* result)
{
    assert( bytes );

    *result = (uint16_t)bytes[0] | (uint16_t)bytes[1] << 8;

    return bytes + sizeof(uint16_t);
}



static uint8_t* writeLittle16(uint8_t* bytes, uint16_t value)
{
    assert( bytes );

    bytes[0] = (uint8_t)(value & 0xFF);
    bytes[1] = (uint8_t)((value >> 8) & 0xFF);

    return bytes + sizeof(uint16_t);
}



static uint8_t* writeLittle32(uint8_t* bytes, uint32_t value)
{
    assert( bytes );

    bytes[0] = (uint8_t)(value & 0xFF);
    bytes[1] = (uint8_t)((value >> 8) & 0xFF);
    bytes[2] = (uint8_t)((value >> 16) & 0xFF);
    bytes[3] = (uint8_t)((value >> 24) & 0xFF);

    return bytes + sizeof(uint32_t);
}



static void printMessageProperties(MessageHandler_MessageProperties* messageProperties)
{
    assert( messageProperties );

    printf("    Message Properties: 0x%04X\r\n", messageProperties->value);
    printf("      priority:       %d\r\n", messageProperties->priority);
    printf("      ackDesignation: %d\r\n", messageProperties->ackDesignation);
    printf("      version:        %d\r\n", messageProperties->version);
}



static void printHeader(MessageHandler_Header* header)
{
    cout << "  Header:" << endl;
    printMessageProperties(&header->properties);
    cout << "    Command Code:   0x" << hex << header->commandCode << endl;
    cout << "    Payload Length: "   << dec << header->payloadLength << endl;
}



static void printHeartbeat(MessageHandler_HeartbeatPayload* heartbeat)
{
    assert( heartbeat );

    cout << "    Heartbeat:" << endl;
    cout << "      Epoch Time:    "   << dec<< heartbeat->epochTime_seconds << " seconds" << endl;
    cout << "      Serial Number: 0x" << hex << heartbeat->serialNumber << endl;
    cout << "      Voltage:       "   << dec << heartbeat->voltage_cV << " cV" << endl;
    cout << "      Temperature:   "   << dec << +heartbeat->temperature_C << " degrees C" << endl;

    if( heartbeat->mode == 0 )
        cout << "      Mode:          " << "Standby" << endl;
    else 
        cout << "      Mode:          " << "SAR" << endl;
}



static void printPayload(MessageHandler_Payload* payload, uint16_t payloadType)
{
    assert( payload );

    cout << "  Payload:" << endl;

    if( payloadType == MESSAGE_HANDLER_COMMAND_SETSARMODE )
    {
        char* string = cJSON_Print(payload->json);
        cout << string << endl;
    }
    else if( payloadType == MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE )
    {
        cout << "    Enable Standby State: " << payload->enableStandby << endl;
    }
    else if( payloadType == MESSAGE_HANDLER_COMMAND_HEARTBEAT )
    {
        printHeartbeat(&payload->heartbeat);
    }
}



static uint16_t generateChecksum(uint8_t* buffer, uint32_t size)
{
    assert( buffer );

    uint16_t checksum = 0;

    for(uint32_t i = 0; i < size; i++)
    {
        checksum += buffer[i];
    }

    return checksum;
}



MessageHandler::MessageHandler()
{
    this->parseIndex         = 0;

    this->headerChecksum     = 0;
    this->payloadChecksum    = 0;

    this->header.commandCode      = 0;
    this->header.properties.value = 0;
    this->header.payloadLength    = 0;

    this->payload.json = NULL;
    
    this->serializedMessage  = NULL;
    this->serializedSize     = 0;
}

MessageHandler::~MessageHandler()
{
    if( this->serializedMessage )
        free(this->serializedMessage);

    if( this->payload.json )
        cJSON_free(this->payload.json);
}

MessageHandler::MessageHandler(uint8_t* rawBuffer, uint32_t size)
{
    this->parseIndex         = 0;

    this->headerChecksum     = 0;
    this->payloadChecksum    = 0;

    this->header.commandCode      = 0;
    this->header.properties.value = 0;
    this->header.payloadLength    = 0;

    this->payload.json = NULL;
    
    this->serializedMessage  = NULL;
    this->serializedSize     = 0;

    this->parseBytes(rawBuffer, size, NULL);
}

/*
* @brief Outputs the message in human-readable format to stdout
*/
void MessageHandler::print(void)
{   
    cout << "Message:" << endl;
    cout << "  Key Signature:    " << this->packetSignature << endl;
    cout << "  Header Checksum:  0x" << hex << this->headerChecksum << endl;
    cout << "  Payload Checksum: 0x" << hex << this->payloadChecksum << endl;
    printHeader(&this->header);
    printPayload(&this->payload, this->header.commandCode);
}

/*
* @brief: Parse a single byte as part of a stream of bytes
*
* @param[out] byte - pointer to array to add to parseBuffer
* @return     full message compiled
*/
bool MessageHandler::parseByte(char byte)
{
    this->parseBuffer[this->parseIndex++] = byte;

    if( this->parseIndex <= fieldSize_keySignature )
    {
        if( (char)byte != 'T' )
        {
            this->parseIndex = 0;
        }
        else if( this->parseIndex == fieldSize_keySignature )
        {
            cout << "Receiving Message:" << endl;
            cout << "  Key Signature:    " << this->packetSignature << endl;
        }
    }
    else if( this->parseIndex == (fieldIndex_headerChecksum + fieldSize_headerChecksum) )
    {
        readLittle16(&this->parseBuffer[fieldIndex_headerChecksum], &this->headerChecksum);
        cout << "  Header Checksum:  0x" << hex << this->headerChecksum << endl;
    }
    else if( this->parseIndex == (fieldIndex_dataChecksum + fieldSize_dataChecksum) )
    {
        readLittle16(&this->parseBuffer[fieldIndex_dataChecksum], &this->payloadChecksum);
        cout << "  Payload Checksum: 0x" << hex << this->payloadChecksum << endl;
    }
    else if( this->parseIndex == (fieldIndex_messageProperties + fieldSize_messageProperties) )
    {
        readLittle16(&this->parseBuffer[fieldIndex_messageProperties], &this->header.properties.value);

        cout << "  Header:" << endl;

        printMessageProperties(&this->header.properties);
    }
    else if( this->parseIndex == (fieldIndex_commandCode + fieldSize_commandCode) )
    {
        readLittle16(&parseBuffer[fieldIndex_commandCode], &this->header.commandCode);

        if(   this->header.commandCode != MESSAGE_HANDLER_COMMAND_SETSARMODE 
           && this->header.commandCode != MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE 
           && this->header.commandCode != MESSAGE_HANDLER_COMMAND_HEARTBEAT
          )
        {
            printf("Error - Invalid command code: 0x%04X\r\n\r\n", this->header.commandCode);
            this->parseIndex = 0;
        }
        else
        {
            printf("    Command Code:   0x%04X\r\n", this->header.commandCode);
        }
    }
    else if( this->parseIndex == (fieldIndex_payloadSize + fieldSize_payloadSize) )
    {
        readLittle16(&this->parseBuffer[fieldIndex_payloadSize], &this->header.payloadLength);

        cout << "    Payload Length: " << dec << this->header.payloadLength << endl;

        if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE && this->header.payloadLength != sizeof(uint8_t) )
        {
            printf("Error - invalid payload size for \"Set Standby State\" message\r\n\r\n");
            this->parseIndex = 0;
        }
        else if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_HEARTBEAT && this->header.payloadLength != sizeof(MessageHandler_HeartbeatPayload) )
        {
            printf("Error - invalid payload size for \"Heartbeat\" message\r\n\r\n");
            this->parseIndex = 0;
        }

        uint16_t headerChecksum = generateChecksum(this->header.headerBytes, messageHandler_headerSize);

        if( headerChecksum != this->headerChecksum )
        {
            printf("Error - invalid header checksum; discontinuing parse\r\n\r\n");
            this->parseIndex = 0;
        }
    }
    else if( this->parseIndex == (uint32_t)(fieldIndex_payload + this->header.payloadLength) )
    {
        bool messageValid = true;

        uint16_t payloadChecksum = generateChecksum(&this->parseBuffer[fieldIndex_payload], this->header.payloadLength);

        if( payloadChecksum != this->payloadChecksum )
        {
            printf("Error - invalid payload checksum (0x%X != 0x%X)\r\n\r\n", payloadChecksum, this->payloadChecksum);
            this->parseIndex = 0;
            messageValid = false;
        }

        if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_SETSARMODE )
        {
            messageValid = this->setPayloadJson((char*)&this->parseBuffer[fieldIndex_payload]);
            if( !messageValid )
            {
                printf("Error - invalid JSON in \"Set Sar Mode\" message\r\n");
                cout << endl << "  " << (char*)&this->parseBuffer[fieldIndex_payload] << endl;
            }
        }
        else if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE )
        {
            this->setPayloadStandbyEnabled(this->parseBuffer[fieldIndex_payload]);
        }
        else if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_HEARTBEAT )
        {
            this->setHeartbeat((MessageHandler_HeartbeatPayload*)&this->parseBuffer[fieldIndex_payload]);
        }

        if( messageValid )
            printPayload(&this->payload, this->header.commandCode);

        this->parseIndex = 0;

        return messageValid;
    }

    return false;
}

/*
* @brief: Parse bytes as they come in, handles multiple calls
*
* @param[in] buffer           - pointer to array to add to parseBuffer
* @param[in]  size            - size of the input buffer
* @param[out] remainingBuffer - a pointer to the remaining items in the buffer if a full message is found
* @return     full message compiled
*/
bool MessageHandler::parseBytes(uint8_t* buffer, uint32_t size, uint8_t** remainingBuffer)
{
    for(uint32_t j = 0; j < size; j++)
    {
        if( this->parseByte(buffer[j]) )
        {
            if( remainingBuffer && (size > j + 1) )
            {
                *remainingBuffer = &buffer[j + 1];
            }

            return true;
        }
    }

    return false;
}

/*
* @brief: Serialize a message built by originally
*
* @param[out] buffer - this is a double pointer to be able return a new pointer to the serialization of object
* @return     size   - serialization size
*/
uint32_t MessageHandler::getSerialized(uint8_t** buffer)
{
    if( this->serializedMessage != NULL )
        free(this->serializedMessage);

    this->serializedSize = messageHandler_headerSize + messageHandler_prefixSize + this->header.payloadLength;

    this->serializedMessage = (uint8_t*)malloc(this->serializedSize);

    this->headerChecksum = generateChecksum(this->header.headerBytes, messageHandler_headerSize);

    writeLittle16(&this->serializedMessage[fieldIndex_keySignature], *(uint16_t*)this->packetSignature);
    writeLittle16(&this->serializedMessage[fieldIndex_headerChecksum], this->headerChecksum);
    writeLittle16(&this->serializedMessage[fieldIndex_messageProperties], this->header.properties.value);
    writeLittle16(&this->serializedMessage[fieldIndex_commandCode], this->header.commandCode);
    writeLittle16(&this->serializedMessage[fieldIndex_payloadSize], this->header.payloadLength);

    if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_SETSARMODE )
    {
        strncpy((char*)&this->serializedMessage[fieldIndex_payload], 
                cJSON_PrintUnformatted(this->payload.json),
                this->header.payloadLength
               );
    }
    else if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE )
    {
        this->serializedMessage[fieldIndex_payload] = (uint8_t)this->payload.enableStandby;
    }
    else if( this->header.commandCode == MESSAGE_HANDLER_COMMAND_HEARTBEAT )
    {
        uint8_t* nextPtr = writeLittle32(&this->serializedMessage[fieldIndex_payload], this->payload.heartbeat.epochTime_seconds);
        nextPtr          = writeLittle32(nextPtr, this->payload.heartbeat.epochTime_seconds);
        nextPtr          = writeLittle32(nextPtr, this->payload.heartbeat.serialNumber);
        nextPtr          = writeLittle16(nextPtr, this->payload.heartbeat.voltage_cV);
        *nextPtr++       = (uint8_t)this->payload.heartbeat.temperature_C;
        *nextPtr         = this->payload.heartbeat.mode;
    }
    else
    {
        free(this->serializedMessage);
    }

    this->payloadChecksum = generateChecksum(&this->serializedMessage[fieldIndex_payload], this->header.payloadLength);
    writeLittle16(&this->serializedMessage[fieldIndex_dataChecksum], this->payloadChecksum);

    *buffer = this->serializedMessage;
    
    return this->serializedSize;
}



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
void MessageHandler::setHeartbeat(MessageHandler_HeartbeatPayload* heartbeat)
{
    assert( heartbeat );
    this->header.commandCode   = MESSAGE_HANDLER_COMMAND_HEARTBEAT;
    this->header.payloadLength = sizeof(MessageHandler_HeartbeatPayload);
    memcpy(&this->payload.heartbeat, heartbeat, sizeof(MessageHandler_HeartbeatPayload));
}

/*
* @brief Get the heartbeat member data fields of the message
* 
* @param[out] epochSeconds  - epoch time in seconds
* @param[out] serialNumber  - serial number of device
* @param[out] votage_V      - voltage measured in hundredths of a volt (centivolts)
* @param[out] temperature_C - 
* @param[out] mode          -
*/
void MessageHandler::getHeartbeat(MessageHandler_HeartbeatPayload* heartbeat)
{
    assert( heartbeat );

    memcpy(heartbeat, &this->payload.heartbeat, sizeof(MessageHandler_HeartbeatPayload));
}

/*
* @brief Set the message type to JSON string and set the data with the given string
*        This does not validate that the json string is valid JSON
*        TBD: Validate the JSON string or use a json structure like cJSON here instead
*        TBD: Split JSON into seperate fields and set those instead
*
* @param jsonString - JSON string
*/
bool MessageHandler::setPayloadJson(char* jsonString)
{
    #define MAX_STRING_LENGTH 256

    assert( jsonString );

    this->header.commandCode = MESSAGE_HANDLER_COMMAND_SETSARMODE;
    this->payload.json = cJSON_Parse(jsonString);
    this->header.payloadLength = strnlen(jsonString, MAX_STRING_LENGTH);
    if( this->payload.json )
    {
        return true;
    }

    cout << "JSON invalid" << endl;

    return false;
}

/*
* @brief retrieve a pointer the JSON payload of the message
*
* @return JSON string pointer
*/
char* MessageHandler::getPayloadJsonString(void)
{
    if( this->payload.json )
        return cJSON_PrintUnformatted(this->payload.json);
    else
        return NULL;
}

/*
* @brief set the message type to enable/disable standby state
*
* @param enable - value, to which to set the standby enabled field of the message
*/
void MessageHandler::setPayloadStandbyEnabled(bool enable)
{
    this->header.commandCode    = MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE;
    this->payload.enableStandby = enable;
    this->header.payloadLength  = 1;
}

/*
* @brief retrieve the current value of the standby enable field for the message
*
* @return statndyEnabled
*/
bool MessageHandler::getPayloadStandbyEnabled(void)
{
    return this->payload.enableStandby;
}

/*
* @brief Get the length of the data section of the message
*
* @return message data(payload) length
*/
uint16_t MessageHandler::getPayloadLength(void)
{
    return this->header.payloadLength;
}

/*
* @brief Retrieve the command code that specifies the message data type
*
* @return commandCode
*/
uint16_t MessageHandler::getCommandCode(void)
{
    return this->header.commandCode;
}

/*
* @brief Set the message priority, ack/nack designation and version using the structure
*
* @param properties - pointer to a structure that seperates 16 bit value into needed bit fields
*/
void MessageHandler::setMessageProperties(MessageHandler_MessageProperties* properties)
{
    assert( properties );

    memcpy(&this->header.properties, properties, sizeof(MessageHandler_MessageProperties));
}

/*
* @brief Retrieve the message properties fields
*
* @param[out] pointer to a structure that seperates 16 bit value into needed bit fields must not be NULL
*/
void MessageHandler::getMessageProperties(MessageHandler_MessageProperties* properties)
{
    assert( properties );

    memcpy(properties, &this->header.properties, sizeof(MessageHandler_MessageProperties));
}



// EOF