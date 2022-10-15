#include "xv6.h"
// Shim implementations of things Doom needs but xv6 does not have go here
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