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
int snprintf(char * buf, size_t bufsz, const char * restrict format, ... );
// string.h
int isspace(int c);
int toupper(int c);
char * strdup(const char * str);
char * strrchr(const char * str, char c);
char * strncpy(char * dest, const char *src, size_t sz);
int strncmp(const char * lhs, const char * rhs, size_t sz);
char * strstr(const char * str, const char * substr);
// stdlib.h
int abs(int x);
void * realloc(void * ptr, size_t newsz);
void * calloc(size_t num, size_t size);
#endif