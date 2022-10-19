#include "doomgeneric.h"
#include "xv6.h"
#include "doomkeys.h"

uint32_t* DG_ScreenBuffer = 0;

/*
From evdev
#define KEY_RESERVED		0
#define KEY_ESC			1
#define KEY_1			2
#define KEY_2			3
#define KEY_3			4
#define KEY_4			5
#define KEY_5			6
#define KEY_6			7
#define KEY_7			8
#define KEY_8			9
#define KEY_9			10
#define KEY_0			11
#define KEY_MINUS		12
#define KEY_EQUAL		13
#define KEY_BACKSPACE		14
#define KEY_TAB			15
#define KEY_Q			16
#define KEY_W			17
#define KEY_E			18
#define KEY_R			19
#define KEY_T			20
#define KEY_Y			21
#define KEY_U			22
#define KEY_I			23
#define KEY_O			24
#define KEY_P			25
#define KEY_LEFTBRACE		26
#define KEY_RIGHTBRACE		27
#define KEY_ENTER		28
#define KEY_LEFTCTRL		29
#define KEY_A			30
#define KEY_S			31
#define KEY_D			32
#define KEY_F			33
#define KEY_G			34
#define KEY_H			35
#define KEY_J			36
#define KEY_K			37
#define KEY_L			38
#define KEY_SEMICOLON		39
#define KEY_APOSTROPHE		40
#define KEY_GRAVE		41
#define KEY_LEFTSHIFT		42
#define KEY_BACKSLASH		43
#define KEY_Z			44
#define KEY_X			45
#define KEY_C			46
#define KEY_V			47
#define KEY_B			48
#define KEY_N			49
#define KEY_M			50
#define KEY_COMMA		51
#define KEY_DOT			52
#define KEY_SLASH		53
#define KEY_RIGHTSHIFT		54
#define KEY_KPASTERISK		55
#define KEY_LEFTALT		56
#define KEY_SPACE		57
#define KEY_CAPSLOCK		58
#define KEY_F1			59
#define KEY_F2			60
#define KEY_F3			61
#define KEY_F4			62
#define KEY_F5			63
#define KEY_F6			64
#define KEY_F7			65
#define KEY_F8			66
#define KEY_F9			67
#define KEY_F10			68
#define KEY_NUMLOCK		69
#define KEY_SCROLLLOCK		70
#define KEY_KP7			71
#define KEY_KP8			72
#define KEY_KP9			73
#define KEY_KPMINUS		74
#define KEY_KP4			75
#define KEY_KP5			76
#define KEY_KP6			77
#define KEY_KPPLUS		78
#define KEY_KP1			79
#define KEY_KP2			80
#define KEY_KP3			81
#define KEY_KP0			82
#define KEY_KPDOT		83

#define KEY_ZENKAKUHANKAKU	85
#define KEY_102ND		86
#define KEY_F11			87
#define KEY_F12			88
#define KEY_RO			89
#define KEY_KATAKANA		90
#define KEY_HIRAGANA		91
#define KEY_HENKAN		92
#define KEY_KATAKANAHIRAGANA	93
#define KEY_MUHENKAN		94
#define KEY_KPJPCOMMA		95
#define KEY_KPENTER		96
#define KEY_RIGHTCTRL		97
#define KEY_KPSLASH		98
#define KEY_SYSRQ		99
#define KEY_RIGHTALT		100
#define KEY_LINEFEED		101
#define KEY_HOME		102
#define KEY_UP			103
#define KEY_PAGEUP		104
#define KEY_LEFT		105
#define KEY_RIGHT		106
#define KEY_END			107
#define KEY_DOWN		108
#define KEY_PAGEDOWN		109
#define KEY_INSERT		110
#define KEY_DELETE		111
#define KEY_MACRO		112
#define KEY_MUTE		113
#define KEY_VOLUMEDOWN		114
#define KEY_VOLUMEUP		115
#define KEY_POWER		116	// SC System Power Down
#define KEY_KPEQUAL		117
#define KEY_KPPLUSMINUS		118
#define KEY_PAUSE		119
#define KEY_SCALE		120	// AL Compiz Scale (Expose)

#define KEY_KPCOMMA		121
#define KEY_HANGEUL		122
#define KEY_HANGUEL		KEY_HANGEUL
#define KEY_HANJA		123
#define KEY_YEN			124
#define KEY_LEFTMETA		125
#define KEY_RIGHTMETA		126
#define KEY_COMPOSE		127
*/

static unsigned char trantbl[] = {
	// 0
	0,
	KEY_ESCAPE,
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	// 8
	'7',
	'8',
	'9',
	'0',
	KEY_MINUS,
	KEY_EQUALS,
	KEY_BACKSPACE,
	KEY_TAB,
	// 16
	'Q',
	'W',
	'E',
	'R',
	'T',
	'Y',
	'U',
	'I',
	// 24
	'O',
	'P',
	0,
	0,
	KEY_ENTER,
	KEY_FIRE,
	'A',
	'S',
	// 32
	'D',
	'F',
	'G',
	'H',
	'J',
	'K',
	'L',
	0,
	// 40
	0,
	0,
	KEY_RSHIFT, // actually LSHIFT but Doom doesn't define one
	0,
	'Z',
	'X',
	'C',
	'V',
	// 48
	'B',
	'N',
	'M',
	0,
	0,
	0,
	KEY_RSHIFT,
	0,
	// 56
	KEY_LALT,
	KEY_USE,
	0,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	// 64
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	0,
	// 72
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	// 80
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	// 88
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	// 96
	0,
	KEY_RCTRL,
	0,
	0,
	KEY_RALT,
	0,
	0,
	KEY_UPARROW,
	// 104
	0,
	KEY_LEFTARROW,
	KEY_RIGHTARROW,
	0,
	KEY_DOWNARROW,
	0,
	0,
	0,
	// 112
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	KEY_PAUSE,
	// 120
	0,
};

// NOTE! called by i_main.c, which contains the main entry
// After dg_Create returns, DoomMain is called and control transfers away from us
// Don't edit this
void dg_Create()
{
	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	DG_Init();
}

static uint32 * xv6fb;
// BEGIN XV6 IMPL
void DG_Init() {
	// Acquire framebuffer
	xv6fb = acquire_fb();
	if (xv6fb == NULL) {
		printf("cannot acquire xv6 framebuffer\n");
		exit(1);
	}
	// nothing else to do
}

void DG_DrawFrame() {
	memmove(xv6fb,DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
	transfer_fb();
}

void DG_SleepMs(uint32_t ms) {
	sleep(ms);
}

uint32_t DG_GetTicksMs() {
	// This is actually the number of clock ticks; each clock tick is 100ms
	return uptime() * 100;
}

int DG_GetKey(int* pressed, unsigned char* key) {
	// Nothing.
	// Return 1 if there is a keyboard event. *pressed = 1 if key is down
	// *key = the key id pressed
	// Return 0 otherwise
	struct input_event ev = poll_kbd();
	if (ev.type == 0) return 0;
	unsigned char trancode = trantbl[ev.code];
	if (trancode == 255) {
		// not a key we know
		return 0;
	}
	*key = trancode;
	*pressed = ev.value;
	return 1;
}

void DG_SetWindowTitle(const char * title) {} // nop

/*static unsigned char translate_key(uint32 evdev_val) {
	if (evdev_val == 105) return KEY_LEFTARROW;
	if (evdev_val == 106) return KEY_RIGHTARROW;
	if (evdev_val == 103) return KEY_UPARROW;
	if (evdev_val == 108) return KEY_DOWNARROW;
	if (evdev_val == 44) return KEY_FIRE; // Z
	if (evdev_val == 45) return KEY_USE; // X
	if (evdev_val == 46) return KEY_LALT; // C
	if (evdev_val == 28) return KEY_ENTER; // Enter
	if (evdev_val == 1) return KEY_ESCAPE; // Esc
	return 255;
}*/