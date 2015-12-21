#ifndef UTIL_H
#define UTIL_H

#include <makestuff.h>

uint8 *loadFile(const char *name, size_t *length);
bool startsWith(const char *s, const char *p);
void bitSwap(uint32 length, uint8 *buffer);

#endif
