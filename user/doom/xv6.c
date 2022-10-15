#include "xv6.h"
// Shim implementations of things Doom needs but xv6 does not have go here
// Some implementations are done right, others may be cheated only to the extent Doom calls them
// Yet more calls may just be edited out on the Doom side to avoid having to implement them

// stdio.h

extern int stderr = 1; // cursed
// Acts like snprintf, but cheats where it can get away with it
// Based on the xv6 kernel's printf implementation
int snprintf(const char * buf, size_t bufsz, const char * restrict format, ... ) {
	va_list va; // varargs info
	va_start(va, format); // initialise the varargs
	int charbuf = 0; // index into buf, charbuf < bufsz
	int charformat = 0; // index into format, should stop at the null char
	// walk the format string
	for (charformat = 0; format[charformat] != '\0'; charformat++) {
		// get current char in format string
		char c = format[charformat];
		if (c == '%') {
			// if '%', special handling
		} else {
			// normal character, just buffer it

		}
		c = fmt[++i] & 0xff;
		if(c == 0)
		break;
		switch(c){
		case 'd':
		printint(va_arg(ap, int), 10, 1);
		break;
		case 'x':
		printint(va_arg(ap, int), 16, 1);
		break;
		case 'p':
		printptr(va_arg(ap, uint64));
		break;
		case 's':
		if((s = va_arg(ap, char*)) == 0)
			s = "(null)";
		for(; *s; s++)
			consputc(*s);
		break;
		case '%':
		consputc('%');
		break;
		default:
		// Print unknown % sequence to draw attention.
		consputc('%');
		consputc(c);
		break;
		}
	}
	va_end(va);
}

// string.h

int isspace(int c) {
	// 0x09 - horizontal tab
	// 0x20 - space
	// 0x0A - linefeed
	// 0x0B - vertical tab
	// 0x0C - form feed
	// 0x0D - carriage return
	return c == 0x09 || c == 0x20 || c == 0x0A || c == 0x0B || c == 0x0C || c == 0x0D;
}

int toupper(int c) {
	if (c >= 0x61 && c <= 0x7A) {
		// in lowercase range
		// subtract hex 0x20 to get into uppercase range
		return c - 0x20;
	}
	// do nothing otherwise
	return c;
}

char * strdup(const char * str) {
	char * dupstr = malloc(strlen(str) + 1);
	if (dupstr == NULL) return NULL; // no memory
	uint s = 0;
	// iterate until we see a null byte
	while (str[s] != '\0') {
		dupstr[s] = str[s];
		s++;
	}
	// we are at the null byte in source string, write to copy
	dupstr[s] == '\0';
	return dupstr;
}

// stdlib.h

int abs(int x) {
	if (x < 0) return -x;
	return x;
}

// Taken from umalloc.c
// This is an ugly hack to get at the block size
// Don't do this

static union header {
  	struct {
    	union header *ptr;
    	uint size;
  	} s;
	long x;
};

void * realloc(void * ptr, size_t newsz) {
	// UGLY HACK
	union header * alloc_header = (union header *)ptr - 1;
	uint oldsz = alloc_header->s.size;
	// END UGLY HACK
	char * newblk = malloc(newsz);
	if (newblk == NULL) return NULL;
	memmove(newblk,ptr,oldsz);
	free(ptr);
	return newsz;
}
void * calloc(size_t num, size_t size) {
	size_t blksz = num * size;
	// check overflow of blksz
	char * blk = malloc(blksz);
	if (blk == NULL) return NULL;
	memset(blk,0,blksz);
	return blk;
}