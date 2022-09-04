#ifndef OSEvent_HEADER_GUARD
#define OSEvent_HEADER_GUARD

#include <stdint.h>
#include <stdbool.h>
#include <UTFString.h>

typedef enum
{
    OS_QUIT,
    OS_RESIZE,
    OS_KEY_PRESS,
    OS_KEY_RELEASE,
    OS_TEXT_INPUT_EVENT,
}OS_EventType;

typedef enum
{
    OS_KMOD_NONE = 0,
    OS_KMOD_LSHIFT = (1 << 0),
    OS_KMOD_RSHIFT = (1 << 1),

    OS_KMOD_LALT   = (1 << 2),
    OS_KMOD_RALT   = (1 << 3),

    OS_KMOD_NUM    = (1 << 4),
    OS_KMOD_CAPS   = (1 << 5),
}OS_Keymod;

#define OS_KMOD_CTRL   (OS_KMOD_LCTRL  | OS_KMOD_RCTRL)
#define OS_KMOD_SHIFT  (OS_KMOD_LSHIFT | OS_KMOD_RSHIFT)
#define OS_KMOD_ALT    (OS_KMOD_LALT   | OS_KMOD_RALT)

//will be added when needed
typedef enum
{
    OS_KEY_A = 'A',
    OS_KEY_B = 'B',
    OS_KEY_C = 'C',
    OS_KEY_D = 'D',
    OS_KEY_E = 'E',
    OS_KEY_F = 'F',
    OS_KEY_G = 'G',
    OS_KEY_H = 'H',
    OS_KEY_I = 'I',
    OS_KEY_J = 'J',
    OS_KEY_K = 'K',
    OS_KEY_L = 'L',
    OS_KEY_M = 'M',
    OS_KEY_N = 'N',
    OS_KEY_O = 'O',
    OS_KEY_P = 'P',
    OS_KEY_Q = 'Q',
    OS_KEY_R = 'R',
    OS_KEY_S = 'S',
    OS_KEY_T = 'T',
    OS_KEY_U = 'U',
    OS_KEY_V = 'V',
    OS_KEY_W = 'W',
    OS_KEY_X = 'X',
    OS_KEY_Y = 'Y',
    OS_KEY_Z = 'Z',

    OS_KEY_a = 'a',
    OS_KEY_b = 'b',
    OS_KEY_c = 'c',
    OS_KEY_d = 'd',
    OS_KEY_e = 'e',
    OS_KEY_f = 'f',
    OS_KEY_g = 'g',
    OS_KEY_h = 'h',
    OS_KEY_i = 'i',
    OS_KEY_j = 'j',
    OS_KEY_k = 'k',
    OS_KEY_l = 'l',
    OS_KEY_m = 'm',
    OS_KEY_n = 'n',
    OS_KEY_o = 'o',
    OS_KEY_p = 'p',
    OS_KEY_q = 'q',
    OS_KEY_r = 'r',
    OS_KEY_s = 's',
    OS_KEY_t = 't',
    OS_KEY_u = 'u',
    OS_KEY_v = 'v',
    OS_KEY_w = 'w',
    OS_KEY_x = 'x',
    OS_KEY_y = 'y',
    OS_KEY_z = 'z',

    OS_KEY_LEFT,
    OS_KEY_RIGHT,
    OS_KEY_UP,
    OS_KEY_DOWN,
    OS_KEY_ENTER,
    OS_KEY_ESC,
    OS_KEY_TAB,
    OS_KEY_BACKSPACE,
}OS_KeySym;

typedef struct
{
    OS_KeySym key_sym; //symbolic key
    OS_Keymod key_mode; //modifier
    bool pressed;
} OS_KeyboardEvent;

typedef struct
{
    uint32_t x;
    uint32_t y;
} OS_ResizeEvent;

typedef struct
{
    UTFStringView text_sv;
} OS_TextInputEvent;

typedef struct
{
    OS_KeyboardEvent keyboard_event;
    OS_TextInputEvent text_input_event;
    OS_ResizeEvent resize_event;
    OS_EventType type;
} OS_Event;

#endif
