#pragma once

void BEGIN_READ(void *buf, int size);
int READ_CHAR(void);
int READ_BYTE(void);
int READ_SHORT(void);
int READ_WORD(void);
int READ_LONG(void);
float READ_FLOAT(void);
char* READ_STRING(void);
float READ_COORD(void);
float READ_ANGLE(void);
float READ_HIRESANGLE(void);