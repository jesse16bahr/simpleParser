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
#include <stdint.h>

#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
    assert( argc >= 2 );

    MessageHandler messageHandler;

    ifstream dataFile;
    dataFile.open (argv[1], ios::binary);

    if( dataFile.is_open() )
    {
        char byte;

        while( !dataFile.eof() )
        {
            dataFile.get(byte);

            if( messageHandler.parseByte(byte) )
            {
                cout << "Full Message Parsed"  << endl << endl;
            }
        }
        dataFile.close();
    }

    return 0;
}





// EOF