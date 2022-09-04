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

#include "OS.h"

#define TEST_FONT "NotoSansKR-Medium.otf"

#define WIDTH 600
#define HEIGHT 600

#include "UTFString.h"
#include "TextBox.h"
#include "TextLine.h"

typedef struct OS
{
    Display* display;
    Window* window;
    int screen;
    XIM xim;
    XIC xic;
    OS_Keymod mod;
} OS;

////////////////////////////////
//IME functions and callbacks
////////////////////////////////
void set_ime_preedit_pos(OS *os, int x, int y)
{
    XPoint spot = {.x = x, .y = y};
    XVaNestedList preedit_attr;
    preedit_attr = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
    XSetICValues(os->xic, XNPreeditAttributes, preedit_attr, NULL);
    XFree(preedit_attr);
}

static int preedit_start_callback(
    XIM xim,
    XPointer client_data,
    XPointer call_data)
{
    printf("preedit start\n");
    // no length limit
    return -1;
}

static void preedit_done_callback(
    XIM xim,
    XPointer client_data,
    XPointer call_data)
{
    printf("preedit done\n");
}

static void preedit_draw_callback(
    XIM xim,
    XPointer client_data,
    XIMPreeditDrawCallbackStruct *call_data)
{

    printf("callback\n");
    XIMText *xim_text = call_data->text;
    if (xim_text != NULL)
    {
        printf("Draw callback string: %s, length: %d, first: %d, caret: %d\n", xim_text->string.multi_byte, call_data->chg_length, call_data->chg_first, call_data->caret);
    }
    else
    {
        printf("Draw callback string: (DELETED), length: %d, first: %d, caret: %d\n", call_data->chg_length, call_data->chg_first, call_data->caret);
    }
}

static void preedit_caret_callback(
    XIM xim,
    XPointer client_data,
    XIMPreeditCaretCallbackStruct *call_data)
{
    printf("preedit caret\n");
    if (call_data != NULL)
    {
        printf("direction: %d position: %d\n", call_data->direction, call_data->position);
    }
}

////////////////////////////////
//End of IME funcitons and callbacks
////////////////////////////////

////////////////////////////////
//Key handling
////////////////////////////////
struct
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

    {XK_Left, OS_KEY_LEFT},
    {XK_Right, OS_KEY_RIGHT},
    {XK_Up, OS_KEY_UP},
    {XK_Down, OS_KEY_DOWN},
    {XK_Escape, OS_KEY_ESC},
    {XK_Tab, OS_KEY_TAB},
    {XK_BackSpace, OS_KEY_BACKSPACE},
    {XK_Return, OS_KEY_ENTER}
};


int main(int argc, char* argv[])
{
    text_line_test();
    utf_test();

    bool init_success = true;

    ////////////////////////////////
    // init x11
    ////////////////////////////////

    OS os;

    /* fallback to LC_CTYPE in env */
    setlocale(LC_CTYPE, "");
    /* implementation-dependent behavior, on my machine it defaults to
     * XMODIFIERS in env */
    XSetLocaleModifiers("");

    /* setting up a simple window */
    os.display = XOpenDisplay(NULL);
    os.screen = DefaultScreen(os.display);
    os.window = XCreateSimpleWindow(os.display,
                                    XDefaultRootWindow(os.display),
                                    0, 0, WIDTH, HEIGHT, 5,
                                    BlackPixel(os.display, os.screen),
                                    BlackPixel(os.display, os.screen));
    XMapWindow(os.display, os.window);

    /* initialize IM and IC */
    os.xim = XOpenIM(os.display, NULL, NULL, NULL);

    XIMCallback draw_callback;
    draw_callback.client_data = NULL;
    draw_callback.callback = (XIMProc)preedit_draw_callback;
    XIMCallback start_callback;
    start_callback.client_data = NULL;
    start_callback.callback = (XIMProc)preedit_start_callback;
    XIMCallback done_callback;
    done_callback.client_data = NULL;
    done_callback.callback = (XIMProc)preedit_done_callback;
    XIMCallback caret_callback;
    caret_callback.client_data = NULL;
    caret_callback.callback = (XIMProc)preedit_caret_callback;
    XVaNestedList preedit_attributes = XVaCreateNestedList(
                                           0,
                                           XNPreeditStartCallback, &start_callback,
                                           XNPreeditDoneCallback, &done_callback,
                                           XNPreeditDrawCallback, &draw_callback,
                                           XNPreeditCaretCallback, &caret_callback,
                                           NULL);

    os.xic = XCreateIC(os.xim,
                       /* the following are in attr, val format, terminated by NULL */
                       XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                       XNClientWindow, os.window,
                       XNPreeditAttributes, preedit_attributes,
                       NULL);
    /* focus on the only IC */
    XSetICFocus(os.xic);
    /* capture the input */
    XSelectInput(os.display, os.window, KeyPressMask | KeyReleaseMask);

    Atom wmDeleteMessage = XInternAtom(os.display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(os.display, os.window, &wmDeleteMessage, 1);

    ////////////////////////////////
    // load font
    ////////////////////////////////
    if (TTF_Init() < 0)
    {
        printf("ERROR: Failed to initializing SDL_TTF: %s\n", SDL_GetError());
        init_success = false;
    }

    TTF_Font* font = TTF_OpenFont(TEST_FONT, 20);
    if (!font)
    {
        printf("ERROR: Failed to load font: %s. %s\n", TEST_FONT,SDL_GetError());
        init_success = false;
    }
    ////////////////////////////////
    // create text box
    ////////////////////////////////

    SDL_Color bg_color = { .r = 30, .g = 30, .b = 30, .a = 255 };
    SDL_Color selection_bg = { .r = 240, .g = 240, .b = 240, .a = 255 };
    SDL_Color selection_fg = { .r = 40, .g = 40, .b = 40, .a = 255 };
    SDL_Color text_color = { .r = 240, .g = 240, .b = 240, .a = 255 };
    SDL_Color cursor_color = { .r = 240, .g = 240, .b = 240, .a = 255 };

    TextBox* box = text_box_create("", WIDTH, HEIGHT, font,
                                   bg_color, text_color, selection_bg, selection_fg, cursor_color,
                                   NULL);

    if (!box)
    {
        printf("ERROR: Failed to create text box\n");
        init_success = false;
    }

    ////////////////////////////////
    // create ximage
    ////////////////////////////////
    XImage *ximage = XCreateImage(os.display, DefaultVisual(os.display, os.screen), DefaultDepth(os.display, os.screen),
                                 ZPixmap, 0, box->render_surface->pixels, WIDTH, HEIGHT, 32, 0);

    ////////////////////////////////
    // event loop
    ////////////////////////////////

    if (!init_success)
    {
        goto cleanup;
    }

    text_box_render(box);
    XPutImage(os.display, os.window, XDefaultGC(os.display, os.screen), ximage, 0, 0, 0, 0, WIDTH, HEIGHT);

    size_t char_buffer_size = 1024;
    char* char_buffer = malloc(char_buffer_size );
    UTFString *tmp_str = utf_from_cstr("");
    bool quit = false;

    while(!quit)
    {
        while(XPending(os.display))
        {

            XEvent xevent;
            XNextEvent(os.display, &xevent);

            if (XFilterEvent(&xevent, None)){
                break;
            }

            switch(xevent.type)
            {

                case KeyRelease:
                case KeyPress:
                {
                    //KeySym ksym = XK_VoidSymbol;
                    KeySym ksym;
                    Status status;

                    ksym = XLookupKeysym(&xevent.xkey, 0);

                    ////////////////////////
                    //handle key sym event
                    ////////////////////////
                    OS_KeyboardEvent key_event;
                    key_event.pressed = xevent.type == KeyPress;

                    bool found_keysym = false;
                    size_t key_arr_size = sizeof(X11_KEYSYM_TO_OS_KEYSYM) / sizeof(X11_KEYSYM_TO_OS_KEYSYM[0]);
                    for(size_t i=0; i<key_arr_size; i++){
                        if(X11_KEYSYM_TO_OS_KEYSYM[i].x11_keysym == ksym){
                            key_event.key_sym = X11_KEYSYM_TO_OS_KEYSYM[i].os_keysym;
                            found_keysym = true;
                            break;
                        }
                    }
                    if(found_keysym){
                        key_event.key_mode = OS_KMOD_NONE;
                        OS_Event key_event_wraped;
                        key_event_wraped.type = xevent.type == KeyPress ? OS_KEY_PRESS : OS_KEY_RELEASE;
                        key_event_wraped.keyboard_event = key_event;

                        text_box_handle_event(box, &key_event_wraped);
                    }


                    ////////////////////////
                    //handle text input event
                    ////////////////////////

                    size_t c = Xutf8LookupString(os.xic, &xevent.xkey,
                                                 char_buffer, char_buffer_size - 1,
                                                 &ksym, &status);
                    if (status == XBufferOverflow)
                    {
                        char_buffer_size = c + 1;
                        char_buffer = realloc(char_buffer, char_buffer_size);
                        c = Xutf8LookupString(os.xic, &xevent.xkey,
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
                            text_box_handle_event(box, &input_event_wrapped);
                        }
                    }



                }

                case Expose:
                {
                }
                break;

                case ClientMessage:
                {
                    if(xevent.xclient.data.l[0] == wmDeleteMessage)
                    {
                        OS_Event quit_event = {.type = OS_QUIT};
                        text_box_handle_event(box, &quit_event);
                        quit = true;
                    }
                }
                break;

            }
        }
        text_box_render(box);
        XPutImage(os.display, os.window, XDefaultGC(os.display, os.screen), ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
        XFlush(os.display);
    }

    free(char_buffer);
    utf_destroy(tmp_str);
    XDestroyIC(os.xic);
    XDestroyWindow(os.display,os.window);

cleanup :
    if (font)
    {
        TTF_CloseFont(font);
    }
    if (box)
    {
        text_box_destroy(box);
    }
    TTF_Quit();
    SDL_Quit();

    return 0;
}

