/* MessageHandler.h
 *
 * This defines a class that may be used to parse, store and print
 *   packets of data for a coding exercise given by IMSAR.
 * May also be used to create and serialize messages byof this type
 *
 *
 * Copyright 2018 by Jesse Bahr
 *  All rights reserved.
 */

#include "MessageHandler.h"
#include <stdio.h>
#include <assert.h>
#include <cJSON.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

enum
{
    base16 = 16,
    base10 = 10,
};

enum
{
    argvIndex_outFile           = 1,
    argvIndex_messageProperties = 2,
    argvIndex_commandCode       = 3,
    argvIndex_payload           = 4,
};

int main(int argc, char *argv[])
{
    MessageHandler message;

    MessageHandler_MessageProperties properties;

    properties.value     = (uint16_t)strtoul(argv[argvIndex_messageProperties], NULL, base16);
    uint16_t commandCode = (uint16_t)strtoul(argv[argvIndex_commandCode], NULL, base16);

    message.setMessageProperties(&properties);

    printf("%x: ", commandCode);
    printf("%s\r\n", argv[argvIndex_payload]);

    if( commandCode == MESSAGE_HANDLER_COMMAND_SETSARMODE )
    {
        message.setPayloadJson(argv[argvIndex_payload]);
    }
    else if( commandCode == MESSAGE_HANDLER_COMMAND_SETSTANDBYSTATE )
    {
        message.setPayloadStandbyEnabled((bool)argv[argvIndex_payload]);
    }
    else if( commandCode == MESSAGE_HANDLER_COMMAND_HEARTBEAT )
    {
        MessageHandler_HeartbeatPayload heartbeat;
        heartbeat.epochTime_seconds = (uint32_t)strtoul(argv[argvIndex_payload], NULL, base10);
        heartbeat.serialNumber = (uint32_t)strtoul(argv[argvIndex_payload + 1], NULL, base16);
        heartbeat.voltage_cV = (uint32_t)strtoul(argv[argvIndex_payload + 2], NULL, base10);
        heartbeat.temperature_C = (uint32_t)strtoul(argv[argvIndex_payload + 3], NULL, base10);
        heartbeat.mode = (uint32_t)strtoul(argv[argvIndex_payload + 4], NULL, base10);
        
        message.setHeartbeat(&heartbeat);
    }
    else
    {
        assert( false );
    }

    uint8_t* serializedBuffer;
    uint32_t serializedSize = message.getSerialized(&serializedBuffer);

    ofstream dataFile;
    dataFile.open(argv[argvIndex_outFile], ios::out | ios::binary);
    dataFile.write((const char*)serializedBuffer, serializedSize);
    dataFile.flush();
    dataFile.close();

    return 0;
}





// EOF