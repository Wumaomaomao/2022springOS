#ifndef __SERIAL_H__
#define __SERIAL_H__

void initSerial(void);
void putChar(char);
void putString(const char *str);
void putInt(int i);

#define SERIAL_PORT  0x3F8

#endif
