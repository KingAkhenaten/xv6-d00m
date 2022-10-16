#include "xv6.h"
// Shim implementations of things Doom needs but xv6 does not have go here
// Some implementations are done right, others may be cheated only to the extent Doom calls them
// Yet more calls may just be edited out on the Doom side to avoid having to implement them

// stdio.h

int stderr = 1; // cursed

static void snputc(char * buf, size_t bufsz, int * charbufidx, int ch);
static void snputs(char * buf, size_t bufsz, int * charbufidx, const char * str);
static void snputi(char * buf, size_t bufsz, int * charbufidx, int64_t num, int base);
static void snputu(char * buf, size_t bufsz, int * charbufidx, uint64_t num, int base);
static void snputptr(char * buf, size_t bufsz, int * charbufidx, void * ptr);
// Acts like snprintf, but cheats where it can get away with it to only support the calls Doom uses.
// This is NOT a complete snprintf implementation, you WILL run into bugs if you use this expecting it to work like snprintf in the stdlibs
// Based on the xv6 kernel's printf implementation
int snprintf(char * buf, size_t bufsz, const char * restrict format, ... ) {
	va_list va; // varargs info
	va_start(va, format); // initialise the varargs
	vsnprintf(buf,bufsz,format,va);
	va_end(va);
	return 0;
}

// Acts like vsnprintf, same caveats as above
int vsnprintf(char * buf, size_t bufsz, const char * restrict format, va_list va) {
	int charbuf = 0; // index into buf, charbuf < bufsz
	int charformat = 0; // index into format, should stop at the null char
	// walk the format string
	for (charformat = 0; format[charformat] != '\0'; charformat++) {
		// get current char in format string
		char c = format[charformat];
		if (c == '%') {
			// if '%', special handling
			charformat++;
			char f = format[charformat];
			if (f == '\0') {
				// string null terminator
				break;
			}
			switch(f) {
				case 'd':
				case 'i':
					// Integer
					{
						int64_t strint = va_arg(va,int64_t);
						snputi(buf,bufsz,&charbuf,strint,10);
					}
					break;
				case 'x':
					// Hex integer
					{
						uint64_t strxint = va_arg(va,uint64_t);
						snputu(buf,bufsz,&charbuf,strxint,16);
					}
					break;
				case 'p':
					// Pointer
					{
						void * strptr = va_arg(va,void *);
						snputptr(buf,bufsz,&charbuf,strptr);
					}
					break;
				case 's':
					// String
					{
						char * strarg = va_arg(va,char *);
						snputs(buf,bufsz,&charbuf,strarg);
					}
					break;
				case '%':
					// Literal %
					snputc(buf,bufsz,&charbuf,'%');
				break;
			default:
				// We don't know this one
				printf("snprintf: illegal placeholder ASCII%d in format string '%s' \n",f,format);
				// Kill the program
				exit(-1);
				return 0;
			}
		} else {
			// normal character, just buffer it
			snputc(buf,bufsz,&charbuf,c);
		}
		
	}
	return 0;
}

static void snputc(char * buf, size_t bufsz, int * charbufidx, int ch) { // assist for the above
	if (*charbufidx < bufsz) {
		buf[*charbufidx] = ch;
		(*charbufidx)++;
	}
}

static void snputs(char * buf, size_t bufsz, int * charbufidx, const char * str) { // assist for the above
	uint i;
	uint len = strlen(str);
	for (i = 0; i < len; i++) {
		snputc(buf,bufsz,charbufidx,str[i]);
	}
}

static const char * digits = "0123456789abcdef";

static void snputi(char * buf, size_t bufsz, int * charbufidx, int64_t num, int base) {
	// Numbers have to be generated in a stack since we only can peel off the bottom
	char numbuf[32];
	int numbufidx = 0;
	int64_t n = num;
	int negative = n < 0; // if true, append - to end
	// peel and buffer
	do {
		numbuf[numbufidx] = digits[abs(n % base)];
		numbufidx++;
		n = n / base;
	} while (n > 0);

	if (negative) {
		numbuf[numbufidx] = '-';
		numbufidx++;
	}
	// now pop the stack
	while (numbufidx > 0) {
		numbufidx--;
		snputc(buf,bufsz,charbufidx,numbuf[numbufidx]);
	}
}

static void snputu(char * buf, size_t bufsz, int * charbufidx, uint64_t num, int base) {
	// Unsigned version
	// Numbers have to be generated in a stack since we only can peel off the bottom
	char numbuf[32];
	int numbufidx = 0;
	uint64_t n = num;
	// peel and buffer
	do {
		numbuf[numbufidx] = digits[n % base];
		numbufidx++;
		n = n / base;
	} while (n > 0);
	// now pop the stack
	while (numbufidx > 0) {
		numbufidx--;
		snputc(buf,bufsz,charbufidx,numbuf[numbufidx]);
	}
}

static void snputptr(char * buf, size_t bufsz, int * charbufidx, void * ptr) { // assist for the above
	uint64 ptrval = (uint64) ptr;
	snputc(buf,bufsz,charbufidx,'0');
	snputc(buf,bufsz,charbufidx,'x');
	int i;
	for (i = 0; i < 16; i++) { // 16 digits in a 64 bit value
		snputc(buf,bufsz,charbufidx,digits[(ptrval >> ((15 - i) * 4)) & 0xF]);
	}
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
	dupstr[s] = '\0';
	return dupstr;
}

char * strrchr(const char * str, char c) {
	char * ptr = (char *)str + strlen(str) - 1;
	while (ptr > str) {
		if(*ptr == c) return ptr;
		ptr--;
	}
	if(*ptr == c) return ptr; // to avoid rolling over for the zero pointer (this should not happen though)
	return NULL;
}
char * strncpy(char * dest, const char *src, size_t sz) {
	size_t i = 0;
	int hitnull = 0;
	while (i < sz) {
		if (src[i] == '\0') hitnull = 1;
		if (hitnull) {
			dest[i] = '\0';
		} else {
			dest[i] = src[i];
		}
		i++;
	}
	return dest;
}

int strncmp(const char * lhs, const char * rhs, size_t sz) {
	size_t i = 0;
	while (lhs[i] == rhs[i] && i < sz) i++;
	return (unsigned char)lhs[i] - (unsigned char)rhs[i];
}

char * strstr(const char * str, const char * substr) {
	uint strl = strlen(str);
	uint substrl = strlen(substr);
	if (substrl == 0) return (char *) str; // empty search string always succeeds
	if (substrl > strl) return NULL; // search string longer than haystack string, always fails
	size_t hayidx;
	size_t needleidx;
	// search all positions that are searchable
	// str 3, needle 2 = pos 0 1
	// str 5, needle 3 = pos 0 1 2
	for (hayidx = 0; hayidx <= strl - substrl; hayidx++) {
		// iterate the needle string indices
		for (needleidx = 0; needleidx < substrl; needleidx++) {
			// if mismatch, continue outer loop
			if (str[hayidx + needleidx] != substr[needleidx]) {
				goto continue_outer;
			}
		}
		// if we get here, we iterated the whole substring; success
		return (char *) &str[hayidx];
		continue_outer: {} // block here to make compiler happy
	}
	// no success
	return NULL;
}

// stdlib.h

int abs(int x) {
	if (x < 0) return -x;
	return x;
}

// Taken from umalloc.c
// This is an ugly hack to get at the block size
// Don't do this

union header {
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
	return (void *) newblk;
}
void * calloc(size_t num, size_t size) {
	size_t blksz = num * size;
	// check overflow of blksz
	char * blk = malloc(blksz);
	if (blk == NULL) return NULL;
	memset(blk,0,blksz);
	return blk;
}