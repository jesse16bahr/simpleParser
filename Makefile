# Makefile for coding exercise to practice building and parsing messages
# This builds messageParser.exe
# messageGenerator.exe is an somewhat easy way to generate binary files
#  for messageParser.exe to parse

CC       = g++
CPPFLAGS = -g -Wall -I.
SUBDIRS  = cJSON

all: build/messageParser.exe build/messageGenerator.exe

build/messageGenerator.exe: build/MessageHandler.o build/messageGenerator.o build/cJSON.o
	$(CC) $(CPPFLAGS) -o build/messageGenerator.exe build/MessageHandler.o build/messageGenerator.o build/cJSON.o

build/messageParser.exe: build/MessageHandler.o build/messageParser.o build/cJSON.o
	$(CC) $(CPPFLAGS) -o build/messageParser.exe build/MessageHandler.o build/messageParser.o build/cJSON.o

build/MessageHandler.o: MessageHandler.cpp MessageHandler.h
	$(CC) $(CPPFLAGS) -c MessageHandler.cpp -o build/MessageHandler.o

build/messageGenerator.o: messageGenerator.cpp
	$(CC) $(CPPFLAGS) -c messageGenerator.cpp -o build/messageGenerator.o

build/messageParser.o: messageParser.cpp
	$(CC) $(CPPFLAGS) -c messageParser.cpp -o build/messageParser.o

build/cJSON.o: cJSON.c cJSON.h
	$(CC) $(CPPFLAGS) -c cJSON.c -o build/cJSON.o


clean:
	rm build/*