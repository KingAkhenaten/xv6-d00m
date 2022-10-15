#ifndef _XV6_H_
#define _XV6_H_
// Include guard because xv6 doesn't for some reason
// Breaks Doom because of multiple declarations but xv6 itself depends on this
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Typedefs Doom expects to exist, which are not identical to the ones xv6 uses
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int  uint32_t;
typedef unsigned long uint64_t;
// More things

// Standard error, to placate *printf functions that use it
extern int stderr;
#endif