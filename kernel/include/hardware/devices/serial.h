#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stddef.h>

#define COM1 0x3F8

bool initSerial();

bool isSerialReceived();
char readSerial();
char* readSerialString(char* buffer, size_t len);

bool isTransmitEmpty();
void writeSerial(char a);
void writeSerialString(const char* str);

#endif