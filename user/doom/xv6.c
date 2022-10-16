#include "xv6.h"
// Shim implementations of things Doom needs but xv6 does not have go here
// Some implementations are done right, others may be cheated only to the extent Doom calls them
// Yet more calls may just be edited out on the Doom side to avoid having to implement them

// stdio.h

int stderr = 1; // Yes, this is wrong. But for compatibility...

// Assist calls
// With buf, with the given size, at the given index, put the char/str/num/unsigned/ptr at this location
// and increment the index. If there is not enough room, place as many chars as will fit.
static void snputc(char * buf, size_t bufsz, int * charbufidx, int ch);
static void snputs(char * buf, size_t bufsz, int * charbufidx, const char * str);
static void snputi(char * buf, size_t bufsz, int * charbufidx, int64_t num, int base);
static void snputu(char * buf, size_t bufsz, int * charbufidx, uint64_t num, int base);
static void snputptr(char * buf, size_t bufsz, int * charbufidx, void * ptr);
// Convert a ASCII num to an int, assuming isdigit(c) is true
static int ctoi(int c);
// Like ctoi but hex digits too at the cost of speed
static int ctoix(int c);
// Parse an int/unsigned int in buf, with the specified bufsz, at charbufidx, in the specified base, and put it in num
// Returns 1 on success, 0 otherwise. In any case charbufidx will be advanced
static int sgeti(const char * buf, size_t bufsz, int * charbufidx, int64_t * num, int base);
static int sgetu(const char * buf, size_t bufsz, int * charbufidx, uint64_t * num, int base);

// Acts like snprintf, but cheats where it can get away with it to only support the calls Doom uses.
// This is NOT a complete snprintf implementation, you WILL run into bugs if you use this expecting it to work like snprintf in the stdlibs
// Based on the xv6 kernel's printf implementation
// TODO: Doom uses these special format strings which xv6 does not support the extra modifiers
// "STCFN%.3d"
// "CWILV%2.2d"
// "WIA%d%.2d%.2d"
int snprintf(char * buf, size_t bufsz, const char * restrict format, ... ) {
	va_list va; // varargs info
	va_start(va, format); // initialise the varargs
	vsnprintf(buf,bufsz,format,va);
	va_end(va);
	return 0;
}

// Acts like vsnprintf, same caveats as above
int vsnprintf(char * buf, size_t bufsz, const char * restrict format, va_list va) {
	int bufidx = 0; // index into buf, bufidx < bufsz
	int formatidx = 0; // index into format, should stop at the null char
	// walk the format string
	for (formatidx = 0; format[formatidx] != '\0'; formatidx++) {
		// get current char in format string
		char c = format[formatidx];
		if (c == '%') {
			// if '%', special handling
			formatidx++;
			char f = format[formatidx];
			if (f == '\0') {
				// string null terminator
				break;
			}
			switch (f) {
				case 'd':
				case 'i':
					// Integer
					{
						int64_t strint = va_arg(va,int64_t);
						snputi(buf,bufsz,&bufidx,strint,10);
					}
					break;
				case 'x':
					// Hex integer
					{
						uint64_t strxint = va_arg(va,uint64_t);
						snputu(buf,bufsz,&bufidx,strxint,16);
					}
					break;
				case 'p':
					// Pointer
					{
						void * strptr = va_arg(va,void *);
						snputptr(buf,bufsz,&bufidx,strptr);
					}
					break;
				case 's':
					// String
					{
						char * strarg = va_arg(va,char *);
						snputs(buf,bufsz,&bufidx,strarg);
					}
					break;
				case '%':
					// Literal %
					snputc(buf,bufsz,&bufidx,'%');
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
			snputc(buf,bufsz,&bufidx,c);
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
static const char * digits2 = "0123456789ABCDEF";

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

// Acts like sscanf, but cheats, only supporting the calls Doom uses.
// Seems to only be used for string to int parsing
// sscanf supporting "%x" "%i" " 0x%x" " 0X%x" " 0%o" " %d" -> int. Return value checked ==1 for success in m_misc.c
int sscanf(const char * buf, const char * restrict format, ... ) {
	va_list va;
	va_start(va,format);
	uint bufsz = strlen(buf);
	int bufidx = 0; // index into buf, bufidx < bufsz
	int formatidx = 0; // index into format, should stop at the null char
	int reads = 0; // how many placeholders read into
	// walk the format string
	for (formatidx = 0; format[formatidx] != '\0'; formatidx++) {
		if (bufidx == bufsz) {
			// no more input
			goto end;
		}
		// get current char in format string
		char c = format[formatidx];
		if (c == '%') {
			// if '%', special handling
			formatidx++;
			char f = format[formatidx];
			if (f == '\0') {
				// string null terminator
				goto end;
			}
			switch(f) {
				case 'd':
				case 'i':
					// Integer
					{
						int64_t * strint = va_arg(va,int64_t *);
						if (!sgeti(buf,bufsz,&bufidx,strint,10)) {
							goto end;
						} else {
							reads++;
						}
					}
					break;
				case 'x':
					// Hex unsigned integer
					{
						uint64_t * strxint = va_arg(va,uint64_t *);
						if (!sgetu(buf,bufsz,&bufidx,strxint,16)) {
							goto end;
						} else {
							reads++;
						}
					}
					break;
				case 'o':
					// Octal unsigned integer
					{
						uint64_t * stroint = va_arg(va,uint64_t *);
						if (!sgetu(buf,bufsz,&bufidx,stroint,8)) {
							goto end;
						} else {
							reads++;
						}
					}
					break;
				default:
					// We don't know this one
					printf("sscanf: illegal placeholder ASCII%d in format string '%s' \n",f,format);
					// Kill the program
					exit(-1);
					return 0;
			}
		} else {
			// normal character, expect it or die
			// for whitespace, keep consuming it from the input until it isn't
			if (isspace(buf[bufidx])) {
				while (isspace(buf[bufidx])) bufidx++; // we'll hit the null terminator or more input
			} else {
				if (buf[bufidx] != c) {
					// match fail
					goto end;
				} else {
					// match OK
					bufidx++;
				}
			}
		}
		
	}
	// no more format string left to read
end:
	va_end(va);
	return reads;
}

static int sgeti(const char * buf, size_t bufsz, int * charbufidx, int64_t * num, int base) {
	int64_t integral = 0;
	int negate = 0;
	if (*charbufidx == bufsz) return 0; // no characters
	if (buf[*charbufidx] == '-') {
		negate = 1;
		(*charbufidx)++;
	}
	if (*charbufidx == bufsz) return 0; // still no characters even after the '-'
	// Integral part
	while (*charbufidx < bufsz && isxdigit(buf[*charbufidx])) {
		int64_t nextintegral = base * integral + ctoix(buf[*charbufidx]);
		if (nextintegral < integral) {
			// overflow! return what we have
			*num = negate? -integral : integral;
			(*charbufidx)++; // advance has not happened yet so do so
			return 1;
		}
		integral = nextintegral;
		(*charbufidx)++;
	}
	// return value, charbufidx has advanced already
	*num = negate? -integral : integral;
	return 1;
}

// like sgeti but unsigned
static int sgetu(const char * buf, size_t bufsz, int * charbufidx, uint64_t * num, int base) {
	uint64_t integral = 0;
	if (*charbufidx == bufsz) return 0; // no characters
	// Integral part
	while (*charbufidx < bufsz && isxdigit(buf[*charbufidx])) {
		uint64_t nextintegral = base * integral + ctoix(buf[*charbufidx]);
		if (nextintegral < integral) {
			// overflow! return what we have
			*num = integral;
			(*charbufidx)++; // advance has not happened yet so do so
			return 1;
		}
		integral = nextintegral;
		(*charbufidx)++;
	}
	// return value, charbufidx has advanced already
	*num = integral;
	return 1;
}

// like ctoi but works with hex digits too at cost of speed
static int ctoix(int c) {
	uint digitslen = strlen(digits2);
	size_t i;
	for (i = 0; i < digitslen; i++) {
		if (digits2[i] == toupper(c)) return (int) i;
	}
	return 0;
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

int isdigit(int c) {
	return '0' <= c && c <= '9'; // ASCII abuse
}

int isxdigit(int c) {
	uint digitslen = strlen(digits2);
	size_t i;
	for (i = 0; i < digitslen; i++) {
		if (digits2[i] == toupper(c)) return 1;
	}
	return 0;
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

// Acts like atof, but supports a very limited floating-point format that I hope is "good enough"
// This implementation is hacky, but if I'm honest I don't know a better way to do string-to-FP conversion
// right now
double atof(const char * str) {
	size_t len = strlen(str); // string length
	size_t i = 0; // where in the string we are
	const uint32 FRAC_LIMIT = 1000000000; // The highest power of 10 representable as an int; limit on fractional precision we can parse
	const uint32 FRAC_LIMIT_DIGITS = 9; // Number of zero digits in the above
	uint32 integral = 0;
	uint32 fractional = 0;
	int negate = 0;
	// skip whitespace
	while (isspace(str[i]) && i < len) i++;
	// if at the len, return 0 since no string here
	if (i == len) return 0.0;
	if (str[i] == '-') {
		negate = 1;
		i++;
	}
	if (i == len) return 0.0; // if the string is "-"
	// Integral part
	while (i < len && isdigit(str[i])) {
		int nextintegral = 10 * integral + ctoi(str[i]);
		if (nextintegral < integral) {
			// overflow! return what we have
			double val = integral;
			if (negate) val = -val;
			return val;
		}
		integral = nextintegral;
		i++;
	}
	if (i == len) { // length check, if out of chars, use what we have
		double val = integral;
		if (negate) val = -val;
		return val;
	}
	// Decimal
	if (str[i] != '.') { // not a decimal here, use what we have
		double val = integral;
		if (negate) val = -val;
		return val;
	}
	i++; // otherwise it is a decimal, skip over it
	if (i == len) { // *another length check* i.e. "25."
		double val = integral;
		if (negate) val = -val;
		return val;
	}
	// Fractional part
	int digitlimit = FRAC_LIMIT_DIGITS;
	while (digitlimit > 0 && i < len && isdigit(str[i])) {
		fractional = 10 * fractional + ctoi(str[i]);
		i++;
		digitlimit--;
	}
	// Make final number
	double val = integral;
	if (negate) val = -val;
	val = val + ((double)fractional / (double)FRAC_LIMIT);
	return val;
}

static int ctoi(int c) {
	return c - '0';
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