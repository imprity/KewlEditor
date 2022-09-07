#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <locale.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../OS.h"

#include "UTFString.h"
#include "../TextBox.h"
#include "../TextLine.h"

typedef struct OS
{
    Display* display;
    Window* window;
    int screen;
    XIM xim;
    XIC xic;
    OS_Keymod key_mod;
    size_t window_width;
    size_t window_height;
} OS;

OS* GLOBAL_OS;

TextBox* GLOBAL_BOX;


OS_Keymod os_get_mod_state()
{
    if(!GLOBAL_OS){
        return OS_KMOD_NONE;
    }
    return GLOBAL_OS->key_mod;
}

void os_set_ime_preedit_pos(int x, int y)
{
    if(!GLOBAL_OS){
        return;
    }
    XPoint spot = {.x = x, .y = y};
    XVaNestedList preedit_attributes = XVaCreateNestedList(
                                           0,
                                           XNSpotLocation, &spot,
                                           NULL);
    XSetICValues(GLOBAL_OS->xic, XNPreeditAttributes, preedit_attributes, NULL);
    XFree(preedit_attributes);
}

////////////////////////////////
//Key handling
////////////////////////////////

// this is almost the same thing in SDL x11
static const struct
{
    KeySym x11_keysym;
    OS_KeySym os_keysym;
} X11_KEYSYM_TO_OS_KEYSYM[]=
{
    {XK_A, OS_KEY_A},
    {XK_B, OS_KEY_B},
    {XK_C, OS_KEY_C},
    {XK_D, OS_KEY_D},
    {XK_E, OS_KEY_E},
    {XK_F, OS_KEY_F},
    {XK_G, OS_KEY_G},
    {XK_H, OS_KEY_H},
    {XK_I, OS_KEY_I},
    {XK_J, OS_KEY_J},
    {XK_K, OS_KEY_K},
    {XK_L, OS_KEY_L},
    {XK_M, OS_KEY_M},
    {XK_N, OS_KEY_N},
    {XK_O, OS_KEY_O},
    {XK_P, OS_KEY_P},
    {XK_Q, OS_KEY_Q},
    {XK_R, OS_KEY_R},
    {XK_S, OS_KEY_S},
    {XK_T, OS_KEY_T},
    {XK_U, OS_KEY_U},
    {XK_V, OS_KEY_V},
    {XK_W, OS_KEY_W},
    {XK_X, OS_KEY_X},
    {XK_Y, OS_KEY_Y},
    {XK_Z, OS_KEY_Z},

    {XK_a, OS_KEY_a},
    {XK_b, OS_KEY_b},
    {XK_c, OS_KEY_c},
    {XK_d, OS_KEY_d},
    {XK_e, OS_KEY_e},
    {XK_f, OS_KEY_f},
    {XK_g, OS_KEY_g},
    {XK_h, OS_KEY_h},
    {XK_i, OS_KEY_i},
    {XK_j, OS_KEY_j},
    {XK_k, OS_KEY_k},
    {XK_l, OS_KEY_l},
    {XK_m, OS_KEY_m},
    {XK_n, OS_KEY_n},
    {XK_o, OS_KEY_o},
    {XK_p, OS_KEY_p},
    {XK_q, OS_KEY_q},
    {XK_r, OS_KEY_r},
    {XK_s, OS_KEY_s},
    {XK_t, OS_KEY_t},
    {XK_u, OS_KEY_u},
    {XK_v, OS_KEY_v},
    {XK_w, OS_KEY_w},
    {XK_x, OS_KEY_x},
    {XK_y, OS_KEY_y},
    {XK_z, OS_KEY_z},

    {XK_F1, OS_KEY_F1},
    {XK_F2, OS_KEY_F2},
    {XK_F3, OS_KEY_F3},
    {XK_F4, OS_KEY_F4},
    {XK_F5, OS_KEY_F5},
    {XK_F6, OS_KEY_F6},
    {XK_F7, OS_KEY_F7},
    {XK_F8, OS_KEY_F8},
    {XK_F9, OS_KEY_F9},
    {XK_F10, OS_KEY_F10},
    {XK_F11, OS_KEY_F11},
    {XK_F12, OS_KEY_F12},
    {XK_F13, OS_KEY_F13},
    {XK_F14, OS_KEY_F14},
    {XK_F15, OS_KEY_F15},
    {XK_F16, OS_KEY_F16},
    {XK_F17, OS_KEY_F17},
    {XK_F18, OS_KEY_F18},
    {XK_F19, OS_KEY_F19},
    {XK_F20, OS_KEY_F20},
    {XK_F21, OS_KEY_F21},
    {XK_F22, OS_KEY_F22},
    {XK_F23, OS_KEY_F23},

    {XK_Left, OS_KEY_LEFT},
    {XK_Right, OS_KEY_RIGHT},
    {XK_Up, OS_KEY_UP},
    {XK_Down, OS_KEY_DOWN},
    {XK_Escape, OS_KEY_ESC},
    {XK_Tab, OS_KEY_TAB},
    {XK_BackSpace, OS_KEY_BACKSPACE},
    {XK_Return, OS_KEY_ENTER}
};

//TODO : Perhaps it should read scancode instead of keysym....
//Or not...
static const struct
{
    KeySym x11_keysym;
    OS_Keymod os_keymod;
} X11_KEYSYM_TO_OS_KEYMOD[]=
{
    {XK_VoidSymbol, OS_KMOD_NONE},

    {XK_Shift_L, OS_KMOD_LSHIFT},
    {XK_Shift_R, OS_KMOD_RSHIFT},

    {XK_Alt_L, OS_KMOD_LALT},
    {XK_Alt_R, OS_KMOD_RALT},

    {XK_Num_Lock, OS_KMOD_NUM},
    {XK_Caps_Lock, OS_KMOD_CAPS},
    {XK_Control_L, OS_KMOD_LCTRL},
    {XK_Control_R, OS_KMOD_RCTRL},
};

static bool x11_keysym_to_os_keysym(KeySym xkeysym, OS_KeySym* out_os_keysym)
{
    bool found_keysym = false;
    size_t keysym_arr_size = sizeof(X11_KEYSYM_TO_OS_KEYSYM) / sizeof(X11_KEYSYM_TO_OS_KEYSYM[0]);
    for(size_t i=0; i<keysym_arr_size; i++){
        if(X11_KEYSYM_TO_OS_KEYSYM[i].x11_keysym == xkeysym){
            *out_os_keysym = X11_KEYSYM_TO_OS_KEYSYM[i].os_keysym;
            found_keysym = true;
            break;
        }
    }

    return found_keysym;
}

static bool x11_keysym_to_os_keymod(KeySym xkeysym, OS_Keymod* out_os_keymod)
{
    bool found_keymod = false;
    size_t keymod_arr_size = sizeof(X11_KEYSYM_TO_OS_KEYMOD) / sizeof(X11_KEYSYM_TO_OS_KEYMOD[0]);
    for(size_t i=0; i<keymod_arr_size; i++){
        if(X11_KEYSYM_TO_OS_KEYMOD[i].x11_keysym == xkeysym){
            *out_os_keymod = X11_KEYSYM_TO_OS_KEYMOD[i].os_keymod;
            found_keymod = true;
            break;
        }
    }

    return found_keymod;
}


int linux_main(TextBox* _box, int argc, char* argv[])
{
    GLOBAL_BOX = _box;

    bool init_success = true;

    ////////////////////////////////
    // init x11
    ////////////////////////////////

    OS _os;
    GLOBAL_OS = &_os;

    /* fallback to LC_CTYPE in env */
    setlocale(LC_CTYPE, "");
    /* implementation-dependent behavior, on my machine it defaults to
     * XMODIFIERS in env */
    XSetLocaleModifiers("");

    /* setting up a simple window */
    GLOBAL_OS->display = XOpenDisplay(NULL);
    GLOBAL_OS->screen = DefaultScreen(GLOBAL_OS->display);
    GLOBAL_OS->window = XCreateSimpleWindow(GLOBAL_OS->display,
                                    XDefaultRootWindow(GLOBAL_OS->display),
                                    0, 0, GLOBAL_BOX->w, GLOBAL_BOX->h, 5,
                                    BlackPixel(GLOBAL_OS->display, GLOBAL_OS->screen),
                                    BlackPixel(GLOBAL_OS->display, GLOBAL_OS->screen));

    GLOBAL_OS->window_width = GLOBAL_BOX->w;
    GLOBAL_OS->window_height = GLOBAL_BOX->h;

    if(!GLOBAL_OS->window){
        fprintf(stderr, "%s:%d:Failed to create a x window\n", __FILE__, __LINE__);
        init_success = false;
    }

    XMapWindow(GLOBAL_OS->display, GLOBAL_OS->window);

    /* initialize IM and IC */
    GLOBAL_OS->xim = XOpenIM(GLOBAL_OS->display, NULL, NULL, NULL);

    GLOBAL_OS->xic = XCreateIC(GLOBAL_OS->xim,
                       /* the following are in attr, val format, terminated by NULL */
                       XNInputStyle, XIMPreeditNothing |  XIMStatusNothing,
                       XNClientWindow, GLOBAL_OS->window,
                       NULL);

    if(!GLOBAL_OS->xic){
        fprintf(stderr, "%s:%d:Failed to create a x input cotext\n", __FILE__, __LINE__);
        init_success = false;
    }

    /* focus on the only IC */
    XSetICFocus(GLOBAL_OS->xic);
    /* capture the input */
    XSelectInput(GLOBAL_OS->display, GLOBAL_OS->window,
        KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask);

    Atom wmDeleteMessage = XInternAtom(GLOBAL_OS->display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(GLOBAL_OS->display, GLOBAL_OS->window, &wmDeleteMessage, 1);

    GLOBAL_OS->key_mod = OS_KMOD_NONE;

    ////////////////////////////////
    // create ximage
    ////////////////////////////////
    XImage *ximage = XCreateImage(GLOBAL_OS->display, DefaultVisual(GLOBAL_OS->display, GLOBAL_OS->screen), DefaultDepth(GLOBAL_OS->display, GLOBAL_OS->screen),
                                 ZPixmap, 0, GLOBAL_BOX->render_surface->pixels, GLOBAL_BOX->w, GLOBAL_BOX->h, 32, 0);

    if(!ximage){
        fprintf(stderr, "%s:%d:Failed to create x image\n", __FILE__, __LINE__);
        init_success = false;
    }

    //TODO : For now we use char_buffer and tmp_str to store and parse string that came from xlib event
    //But I think we can handle it using only tmp_str
    size_t char_buffer_size = 1024;
    char* char_buffer = malloc(char_buffer_size );
    UTFString *tmp_str = utf_from_cstr("");

    if (!init_success)
    {
        goto cleanup;
    }

    ////////////////////////////////
    // event loop
    ////////////////////////////////

    text_box_render(GLOBAL_BOX);
    XPutImage(GLOBAL_OS->display, GLOBAL_OS->window, XDefaultGC(GLOBAL_OS->display, GLOBAL_OS->screen), ximage, 0, 0, 0, 0, GLOBAL_BOX->w, GLOBAL_BOX->h);

    bool quit = false;



    while(!quit)
    {

        while(XPending(GLOBAL_OS->display))
        {

            XEvent xevent;
            XNextEvent(GLOBAL_OS->display, &xevent);

            if (XFilterEvent(&xevent, None)){
                break;
            }

            switch(xevent.type)
            {
                case ConfigureNotify:
                {
                    if(xevent.xconfigure.width != GLOBAL_OS->window_width || xevent.xconfigure.height != GLOBAL_OS->window_height){
                        GLOBAL_OS->window_width  = xevent.xconfigure.width;
                        GLOBAL_OS->window_height = xevent.xconfigure.height;

                        text_box_resize(GLOBAL_BOX, GLOBAL_OS->window_width, GLOBAL_OS->window_height);
                        ximage = XCreateImage(GLOBAL_OS->display, DefaultVisual(GLOBAL_OS->display, GLOBAL_OS->screen), DefaultDepth(GLOBAL_OS->display, GLOBAL_OS->screen),
                                 ZPixmap, 0, GLOBAL_BOX->render_surface->pixels, GLOBAL_BOX->w, GLOBAL_BOX->h, 32, 0);
                    }
                }
                case KeyRelease:
                case KeyPress:
                {
                    //KeySym ksym = XK_VoidSymbol;
                    KeySym ksym;
                    Status status;

                    ksym = XLookupKeysym(&xevent.xkey, 0);

                    ////////////////////////
                    //handle key mod
                    ////////////////////////
                    OS_Keymod os_keymod = OS_KMOD_NONE;

                    if(x11_keysym_to_os_keymod(ksym, &os_keymod)){
                        if(xevent.type == KeyPress){
                            GLOBAL_OS->key_mod |= os_keymod;
                        }
                        else{
                            GLOBAL_OS->key_mod &= (~os_keymod);
                        }
                    }

                    ////////////////////////
                    //handle key sym event
                    ////////////////////////
                    OS_KeyboardEvent key_event;
                    key_event.pressed = xevent.type == KeyPress;

                    OS_KeySym os_keysym;

                    if(x11_keysym_to_os_keysym(ksym, &os_keysym)){
                        key_event.key_sym = os_keysym;
                        key_event.key_mode = GLOBAL_OS->key_mod;
                        OS_Event key_event_wraped;
                        key_event_wraped.type = xevent.type == KeyPress ? OS_KEY_PRESS_EVENT : OS_KEY_RELEASE_EVENT;
                        key_event_wraped.keyboard_event = key_event;

                        text_box_handle_event(GLOBAL_BOX, &key_event_wraped);
                    }


                    ////////////////////////
                    //handle text input event
                    ////////////////////////

                    size_t c = Xutf8LookupString(GLOBAL_OS->xic, &xevent.xkey,
                                                 char_buffer, char_buffer_size - 1,
                                                 &ksym, &status);
                    if (status == XBufferOverflow)
                    {
                        char_buffer_size = c + 1;
                        char_buffer = realloc(char_buffer, char_buffer_size);
                        c = Xutf8LookupString(GLOBAL_OS->xic, &xevent.xkey,
                                              char_buffer, char_buffer_size,
                                              &ksym, &status);
                    }
                    if (c)
                    {
                        //null terminate
                        char_buffer[c] = 0;

                        //remove non printable characters
                        //We are only removing control characters(https://www.compart.com/en/unicode/category/Cc)
                        //that ranges (u+0001 u+001f) (u+007f u+009F)
                        //except new line

                        //TODO : maybe check more rigorously

                        utf_set_cstr(tmp_str, char_buffer);
                        size_t char_index = 0;
                        while(char_index < tmp_str->count){
                            UTFStringView sv = utf_sv_sub_str(tmp_str, char_index, char_index+1);
                            uint32_t char_code_at = utf8_to_32(sv.data, sv.data_size);
                            if((char_code_at >= 0x0001 && char_code_at <=0x001f) || (char_code_at >= 0x007f && char_code_at <=0x009f)){
                                utf_erase_range(tmp_str, char_index, char_index+1);
                            }
                            else{
                                char_index++;
                            }
                        }
                        if(tmp_str->count > 0){
                            OS_TextInputEvent input_event;
                            input_event.text_sv =utf_sv_from_str(tmp_str);

                            OS_Event input_event_wrapped = {.text_input_event = input_event, .type = OS_TEXT_INPUT_EVENT};
                            text_box_handle_event(GLOBAL_BOX, &input_event_wrapped);
                        }
                    }

                }

                case Expose:
                {
                    text_box_render(GLOBAL_BOX);
                    bool locked = false;
                    if(SDL_MUSTLOCK(GLOBAL_BOX->render_surface)){
                        locked = true;
                        SDL_LockSurface(GLOBAL_BOX->render_surface);
                    }

                    XPutImage(GLOBAL_OS->display, GLOBAL_OS->window, XDefaultGC(GLOBAL_OS->display, GLOBAL_OS->screen), ximage, 0, 0, 0, 0, GLOBAL_BOX->w, GLOBAL_BOX->h);

                    if(locked){
                        SDL_UnlockSurface(GLOBAL_BOX->render_surface);
                    }
                }
                break;

                case ClientMessage:
                {
                    if(xevent.xclient.data.l[0] == wmDeleteMessage)
                    {
                        OS_Event quit_event = {.type = OS_QUIT_EVENT};
                        text_box_handle_event(GLOBAL_BOX, &quit_event);
                        quit = true;
                    }

                }
                break;
            }
        }
    }

cleanup: ;

    if(char_buffer) {free(char_buffer);}
    if(tmp_str) {utf_destroy(tmp_str);}
    if(GLOBAL_OS->xic) {XDestroyIC(GLOBAL_OS->xic);}
    if(GLOBAL_OS->window) {XDestroyWindow(GLOBAL_OS->display,GLOBAL_OS->window);}

    return 0;
}

