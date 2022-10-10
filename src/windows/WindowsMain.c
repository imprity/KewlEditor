#include <stdbool.h>
#include <stdint.h>
#include <Windows.h>

#include "WindowsMain.h"
#include "../OS.h"
#include "../TextBox.h"

typedef struct OS
{
    OS_Keymod key_mod;
    size_t window_width;
    size_t window_height;
    UTFString* clipboard_paste_text;
    UTFString* clipboard_copy_text;
    HWND hwnd;
    uint16_t pair_high;
    uint16_t pair_low;
    bool received_pair_high;
    bool received_pair_low;
} OS;

OS* GLOBAL_OS;

TextBox* GLOBAL_BOX;

////////////////////////////////
//Key handling
////////////////////////////////

// this is almost the same thing in SDL x11
static const struct
{
    uint64_t v_key;
    OS_KeySym os_keysym;
} VIRTUAL_KEY_TO_OS_KEYSYM[] =
{
    {'A', OS_KEY_A},
    {'B', OS_KEY_B},
    {'C', OS_KEY_C},
    {'D', OS_KEY_D},
    {'E', OS_KEY_E},
    {'F', OS_KEY_F},
    {'G', OS_KEY_G},
    {'H', OS_KEY_H},
    {'I', OS_KEY_I},
    {'J', OS_KEY_J},
    {'K', OS_KEY_K},
    {'L', OS_KEY_L},
    {'M', OS_KEY_M},
    {'N', OS_KEY_N},
    {'O', OS_KEY_O},
    {'P', OS_KEY_P},
    {'Q', OS_KEY_Q},
    {'R', OS_KEY_R},
    {'S', OS_KEY_S},
    {'T', OS_KEY_T},
    {'U', OS_KEY_U},
    {'V', OS_KEY_V},
    {'W', OS_KEY_W},
    {'X', OS_KEY_X},
    {'Y', OS_KEY_Y},
    {'Z', OS_KEY_Z},

    {VK_F1, OS_KEY_F1},
    {VK_F2, OS_KEY_F2},
    {VK_F3, OS_KEY_F3},
    {VK_F4, OS_KEY_F4},
    {VK_F5, OS_KEY_F5},
    {VK_F6, OS_KEY_F6},
    {VK_F7, OS_KEY_F7},
    {VK_F8, OS_KEY_F8},
    {VK_F9, OS_KEY_F9},
    {VK_F10, OS_KEY_F10},
    {VK_F11, OS_KEY_F11},
    {VK_F12, OS_KEY_F12},
    {VK_F13, OS_KEY_F13},
    {VK_F14, OS_KEY_F14},
    {VK_F15, OS_KEY_F15},
    {VK_F16, OS_KEY_F16},
    {VK_F17, OS_KEY_F17},
    {VK_F18, OS_KEY_F18},
    {VK_F19, OS_KEY_F19},
    {VK_F20, OS_KEY_F20},
    {VK_F21, OS_KEY_F21},
    {VK_F22, OS_KEY_F22},
    {VK_F23, OS_KEY_F23},

    {VK_LEFT, OS_KEY_LEFT},
    {VK_RIGHT, OS_KEY_RIGHT},
    {VK_UP, OS_KEY_UP},
    {VK_DOWN, OS_KEY_DOWN},
    {VK_ESCAPE, OS_KEY_ESC},
    {VK_TAB, OS_KEY_TAB},
    {VK_BACK, OS_KEY_BACKSPACE},
    {VK_RETURN, OS_KEY_ENTER}
};

static const struct
{
    uint64_t v_key;
    OS_Keymod os_keymod;
} VIRTUAL_KEY_TO_OS_KEYMOD[] =
{
    {NULL, OS_KMOD_NONE},

    {VK_LSHIFT, OS_KMOD_LSHIFT},
    {VK_RSHIFT, OS_KMOD_RSHIFT},

    {VK_LMENU, OS_KMOD_LALT},
    {VK_RMENU, OS_KMOD_RALT},

    {VK_NUMLOCK, OS_KMOD_NUM},
    {VK_CAPITAL, OS_KMOD_CAPS},
    {VK_LCONTROL, OS_KMOD_LCTRL},
    {VK_RCONTROL, OS_KMOD_RCTRL},

    //for some reason on windows WM_KEYDOWN doesn't emit VK_LSHIFT nor VK_RSHIFT
    //just VK_SHIFT...
    //TODO : find a way to diffrentiate between left shitf, right shift and between left alt and right alt
    {VK_SHIFT, OS_KMOD_SHIFT},
    {VK_CONTROL, OS_KMOD_CTRL},
};

static bool virtual_key_to_os_keysym(uint64_t v_key, OS_KeySym* out_os_keysym)
{
    bool found_keysym = false;
    size_t keysym_arr_size = sizeof(VIRTUAL_KEY_TO_OS_KEYSYM) / sizeof(VIRTUAL_KEY_TO_OS_KEYSYM[0]);
    for (size_t i = 0; i < keysym_arr_size; i++) {
        if (VIRTUAL_KEY_TO_OS_KEYSYM[i].v_key == v_key) {
            *out_os_keysym = VIRTUAL_KEY_TO_OS_KEYSYM[i].os_keysym;
            found_keysym = true;
            break;
        }
    }

    return found_keysym;
}

static bool virtual_key_to_os_keymod(uint64_t v_key, OS_Keymod* out_os_keymod)
{
    bool found_keymod = false;
    size_t keymod_arr_size = sizeof(VIRTUAL_KEY_TO_OS_KEYMOD) / sizeof(VIRTUAL_KEY_TO_OS_KEYMOD[0]);
    for (size_t i = 0; i < keymod_arr_size; i++) {
        if (VIRTUAL_KEY_TO_OS_KEYMOD[i].v_key == v_key) {
            *out_os_keymod = VIRTUAL_KEY_TO_OS_KEYMOD[i].os_keymod;
            found_keymod = true;
            break;
        }
    }

    return found_keymod;
}

void os_set_ime_preedit_pos(int x, int y)
{
    HIMC context = ImmGetContext(GLOBAL_OS->hwnd);

    POINT point = { .x = x, .y = y };
    //according to msdn when you set composition form's dwStyle to CFS_POINT, it only accounts for point
    //I'm putting in rect just to be safe
    RECT rect = { .left = x, .top = y, .right = x + GLOBAL_BOX->w, .bottom = y + GLOBAL_BOX->h };
    COMPOSITIONFORM composition_form = {
        .dwStyle = CFS_POINT,
        .ptCurrentPos = point,
        .rcArea = rect
    };
    ImmSetCompositionWindow(context, &composition_form);
    ImmReleaseContext(GLOBAL_OS->hwnd,context);
}

OS_Keymod os_get_mod_state()
{
    return GLOBAL_OS->key_mod;
}

size_t utf16_strlen(uint16_t* str) {
    size_t len = 0;
    while (str[len] != 0) {
        len++;
    }
    
    return len;
}

void os_request_text_paste()
{
    //handle pasting event
    //TOOD : This is probably not safe we should check if text is malicious or not

    if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return;
    if (!OpenClipboard(GLOBAL_OS->hwnd))
        return;

    HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
    if (hglb != NULL)
    {
        uint16_t *utf16_str = GlobalLock(hglb);
        size_t utf16_size = utf16_strlen(utf16_str) + 1; //include null terminator
        if (utf16_str != NULL)
        {
            uint8_t *utf8_str = NULL;
            size_t utf8_str_size = 0;

            utf16_to_8(utf16_str, utf16_size, utf8_str, &utf8_str_size);
            utf8_str = malloc(utf8_str_size);
            utf16_to_8(utf16_str, utf16_size, utf8_str, &utf8_str_size);

            OS_TextPasteEvent paste_event = { .paste_sv = utf_sv_from_cstr(utf8_str) };
            OS_Event wrapped_paste_event = { .text_paste_event = paste_event, .type = OS_TEXT_PASTE_EVENT };

            text_box_handle_event(GLOBAL_BOX, &wrapped_paste_event);

            GlobalUnlock(hglb);

            free(utf8_str);
        }
    }
    CloseClipboard();
}

UTFString* replace_lf_to_crlf(UTFStringView sv) {
    size_t raw_size = 2;
    while (raw_size < sv.data_size) {
        raw_size *= 2;
    }

    uint8_t* data = malloc(raw_size);
    size_t data_index = 0;

    size_t replace_count = 0;

    for (size_t i = 0; i < sv.data_size; i++) {
        if (data_index + 1 >= raw_size) {
            raw_size *= 2;
            data = realloc(data, raw_size);
        }

        if (sv.data[i] == '\n' &&
            ((i == 0) || (i >= 1 && sv.data[i - 1] != '\r'))
            ) {
            data[data_index++] = '\r';
            data[data_index++] = '\n';

            replace_count++;
        }
        else {
            data[data_index++] = sv.data[i];
        }
    }

    //null terminate
    if (data_index >= raw_size) {
        raw_size *= 2;
        data = realloc(data, raw_size);
    }
    data[data_index++] = 0;

    UTFString* ret_str = malloc(sizeof(UTFString));

    ret_str->data = data;
    ret_str->data_size = data_index;
    ret_str->raw_size = raw_size;
    ret_str->count = sv.count + replace_count;

    return ret_str;
}

void os_set_clipboard_text(UTFStringView sv)
{
    if (!OpenClipboard(GLOBAL_OS->hwnd)) {
        return;
    }

    EmptyClipboard();
    // replace \n to \r\n
    UTFString* copy = replace_lf_to_crlf(sv);

    uint16_t* utf16_str = NULL;
    size_t utf16_str_size = 0;

    //we add one beacuse UTFString's data_size doesn't account null terminator
    utf8_to_16(copy->data, copy->data_size + 1, utf16_str, &utf16_str_size);
    utf16_str = malloc(utf16_str_size * sizeof(uint16_t));
    utf8_to_16(copy->data, copy->data_size + 1, utf16_str, &utf16_str_size);

    // Allocate a global memory object for the text. 

    uint16_t* clipboard_str = GlobalAlloc(GMEM_MOVEABLE, utf16_str_size * sizeof(uint16_t));
    if (clipboard_str == NULL)
    {
        CloseClipboard();
        return FALSE;
    }

    // Lock the handle and copy the text to the buffer. 

    LPTSTR  lock_copy = GlobalLock(clipboard_str);
    memcpy(lock_copy, utf16_str, utf16_str_size * sizeof(uint16_t));
    GlobalUnlock(clipboard_str);

    // Place the handle on the clipboard. 

    SetClipboardData(CF_UNICODETEXT, clipboard_str);

    CloseClipboard();

    //free memories
    free(utf16_str);
    utf_destroy(copy);
    return;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

UTFString* get_windows_system_error_str(DWORD error_code)
{
    LPVOID* lpMsgBuf;

    //get error message formatted in ascii
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        1033, //english_language_id defined at https://learn.microsoft.com/en-us/openspecs/office_standards/ms-oe376/db9b9b72-b10b-4e7e-844c-09f88c972219
        (LPSTR)&lpMsgBuf,
        0, NULL);

    //we are converting it to UTFString for convinience
    //also I'm kinda scared to move string outside of function scope that is allocated by something
    //other than malloc
    UTFString* str = utf_from_cstr(lpMsgBuf);

    LocalFree(lpMsgBuf);
    return str;
}

int windows_main(TextBox* _box, int argc, char* argv[])
{
    GLOBAL_BOX = _box;


    OS _os = { 0 };
    GLOBAL_OS = &_os;
    GLOBAL_OS->received_pair_high = false;
    GLOBAL_OS->received_pair_low = false;
    GLOBAL_OS->key_mod = OS_KMOD_NONE;

    bool init_success = true;

    HINSTANCE hinstance = GetModuleHandle(NULL);
    const wchar_t* class_name = L"Kewl Editor Class";

    WNDCLASS wc = { 0 };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hinstance;
    wc.lpszClassName = class_name;

    RegisterClassW(&wc);

    {
        //before we make window, we first need to calculate window size
        //since CreateWindowEX function's width and height accounts for menu bars
        //and title's size as well..
        RECT tmp_rect = { .left = 0, .right = GLOBAL_BOX->w, .top = 0, .bottom = GLOBAL_BOX->h };
        AdjustWindowRectEx(&tmp_rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

        GLOBAL_OS->hwnd = CreateWindowExW(
            0,
            class_name,
            L"Kewl Editor",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, tmp_rect.right - tmp_rect.left, tmp_rect.bottom - tmp_rect.top,
            NULL,
            NULL,
            hinstance,
            NULL
        );
    }

    if (GLOBAL_OS->hwnd == NULL)
    {
        DWORD error_code = GetLastError();
        UTFString* error_str = get_windows_system_error_str(error_code);
        fprintf(stderr, "%s:%d:ERROR: Failed to create window : %s\n", __FILE__, __LINE__, error_str->data);
        utf_destroy(error_str);
        init_success = false;
    }

    if (!init_success) {
        goto cleanup;
    }

    WINDOWINFO info;

    ShowWindow(GLOBAL_OS->hwnd, SW_SHOW);

    MSG msg = { 0 };

    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

cleanup:

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }break;
    case WM_KEYUP:
    case WM_KEYDOWN:
    {

        ////////////////////////
        //handle key mod
        ////////////////////////
        OS_Keymod os_keymod = OS_KMOD_NONE;

        if (virtual_key_to_os_keymod(wParam, &os_keymod)) {
            if (uMsg == WM_KEYDOWN) {
                GLOBAL_OS->key_mod |= os_keymod;
            }
            else {
                GLOBAL_OS->key_mod &= (~os_keymod);
            }
        }

        ////////////////////////
        //handle key sym event
        ////////////////////////
        OS_KeyboardEvent key_event;
        key_event.pressed = uMsg == WM_KEYDOWN;

        OS_KeySym os_keysym;

        if (virtual_key_to_os_keysym(wParam, &os_keysym)) {
            key_event.key_sym = os_keysym;
            key_event.key_mode = GLOBAL_OS->key_mod;
            OS_Event key_event_wraped;
            key_event_wraped.type = uMsg == WM_KEYDOWN ? OS_KEY_PRESS_EVENT : OS_KEY_RELEASE_EVENT;
            key_event_wraped.keyboard_event = key_event;

            text_box_handle_event(GLOBAL_BOX, &key_event_wraped);
        }
        InvalidateRect(hwnd, NULL, FALSE);
    }break;

    case WM_CHAR:
    {
        bool received_char = false;

        uint16_t utf16_str[3];
        size_t utf16_size = 0;

        if (IS_HIGH_SURROGATE(wParam)) {
            GLOBAL_OS->pair_high = wParam;
            GLOBAL_OS->received_pair_high = true;
        }
        else if (IS_LOW_SURROGATE(wParam)) {
            GLOBAL_OS->pair_low = wParam;
            GLOBAL_OS->received_pair_low = true;
        }
        else {
            //ignore non printable characters
            //We are only removing control characters(https://www.compart.com/en/unicode/category/Cc)
            //that ranges (u+0001 u+001f) (u+007f u+009F)
            //except new line

            //TODO : maybe check more rigorously
            if (!((wParam >= 0x1 && wParam <= 0x1f) || (wParam >= 0x7f && wParam <= 0x9f)))
            {
                utf16_str[0] = wParam;
                utf16_str[1] = 0;
                utf16_size = 2;
                received_char = true;
            }
        }

        if (GLOBAL_OS->received_pair_high && GLOBAL_OS->received_pair_low) {
            utf16_str[0] = GLOBAL_OS->pair_high;
            utf16_str[1] = GLOBAL_OS->pair_low;
            utf16_str[2] = 0;
            utf16_size = 3;
            received_char = true;
            GLOBAL_OS->received_pair_high = false;
            GLOBAL_OS->received_pair_low = false;
        }

        if (received_char) {
            char utf8_buffer[8]; //in theory we only need 4 buffer at max but I'm scared

            size_t str_size = 0;
            //calculate str size
            utf16_to_8(utf16_str, utf16_size, utf8_buffer, &str_size);
            //and convert it
            utf16_to_8(utf16_str, utf16_size, utf8_buffer, &str_size);


            OS_TextInputEvent input_event;
            input_event.text_sv = utf_sv_from_cstr(utf8_buffer);

            OS_Event input_event_wrapped = { .text_input_event = input_event, .type = OS_TEXT_INPUT_EVENT };
            text_box_handle_event(GLOBAL_BOX, &input_event_wrapped);
            //force window to redraw
            InvalidateRect(hwnd, NULL, FALSE);
        }

    }break;

    case WM_PAINT:
    {

        //render text box
        text_box_render(GLOBAL_BOX);

        bool locked = false;
        if (SDL_MUSTLOCK(GLOBAL_BOX->render_surface)) {
            locked = true;
            SDL_LockSurface(GLOBAL_BOX->render_surface);
        }
        HBITMAP bitmap = CreateBitmap(GLOBAL_BOX->w, GLOBAL_BOX->h, 1, 32, GLOBAL_BOX->render_surface->pixels);
        if (locked) {
            SDL_UnlockSurface(GLOBAL_BOX->render_surface);
        }

        if (!bitmap) {
            fprintf(stderr, "%s:%d:ERROR: Failed to create bitmap\n", __FILE__, __LINE__);
        }
        else {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HBITMAP  hOldBitmap;

            HDC hMemDC = CreateCompatibleDC(hdc);
            hOldBitmap = (HBITMAP)SelectObject(hMemDC, bitmap);

            BitBlt(hdc, 0, 0, GLOBAL_BOX->w, GLOBAL_BOX->h, hMemDC, 0, 0, SRCCOPY);

            SelectObject(hMemDC, hOldBitmap);
            DeleteDC(hMemDC);

            DeleteObject(bitmap);

            EndPaint(hwnd, &ps);
        }
        return 0;
    }break;

    case WM_SIZE: {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        text_box_resize(GLOBAL_BOX, width, height);

        //force window to redraw
        InvalidateRect(hwnd, NULL, FALSE);
    }break;

    default:
    {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }break;
    }
    return 0;
}

