#ifndef _XV6_H_
#define _XV6_H_
// Include guard because xv6 doesn't for some reason
// Breaks Doom because of multiple declarations but xv6 itself depends on this
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
// Headers guaranteed to exist in freestanding (that is, no operating system) compiler implementations
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
// I am not sure if these are fully freestanding but I'm going to try it anyway
#include <math.h>

// Standard error, to placate *printf functions that use it
extern int stderr;
#define NULL ((void *)0)
// snprintf from stdio.h
int snprintf(const char * buf, size_t bufsz, const char * restrict format, ... );
// string.h
int isspace(int c);
int toupper(int c);
char * strdup(char * str);
char * strrchr(const char * str, char c);
// stdlib.h
int abs(int x);
#endif