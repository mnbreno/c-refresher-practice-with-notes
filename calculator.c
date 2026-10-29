#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* terminology used in this C and Win32 program

    header: a file of declarations included with #include. windows.h declares
    the Windows GUI types, constants, and functions used by this program.

    macro: a name replaced by the preprocessor before compilation. For example,
    #define ID_ADD 104 lets the code use ID_ADD instead of writing 104 each time.

    control: a child window that provides one part of a GUI, such as a button or
    the number display. A control ID is the integer Windows uses to identify it.

    handle: a Windows-managed reference to a resource. HWND identifies a window;
    HFONT identifies a font object. A handle is not a number to do arithmetic on.

    GDI: Graphics Device Interface, the Windows subsystem for drawing and using
    graphical resources. CreateFontA creates a GDI font object; DeleteObject frees it.

    message: a Windows record describing an event or requested action. WM_CREATE
    reports window creation, WM_COMMAND reports commands from controls or menus,
    and WM_DESTROY reports that a window is being destroyed.

    WPARAM and LPARAM: the first and second extra parameters attached to a Windows
    message. WPARAM is an unsigned pointer-sized integer; LPARAM is a signed
    pointer-sized integer. On this 64-bit build each is 64 bits, but their meaning
    is defined by the particular message, not by whether the value is a number.

    WM_COMMAND fields: for a child-control notification, LOWORD(wParam) extracts
    the low 16 bits containing the control ID, HIWORD(wParam) extracts the next
    16 bits containing the notification code, and lParam contains the child's HWND.
    Menu and accelerator commands use WM_COMMAND too, but do not use lParam this way.

    pointer: a value that stores the address of another value. &answer means the
    address of answer; a double * parameter can receive that address; *result
    accesses the double at that address. This is how calculate writes an answer
    through its output parameter while returning a separate success flag.

    bool: the true/false type from stdbool.h. This program uses it to report
    whether an arithmetic operation succeeded, separately from its numeric result.

    static: at file scope, restricts a name to this source file. Static variables
    also keep their values for the whole time the program is running.

    callback: a function registered with Windows for Windows to call later.
    window_procedure is the callback that receives messages for the main window.

    C string and buffer: a C string is a char array ending with the null byte '\0'.
    char text[64] reserves 64 bytes total, so up to 63 bytes can hold text and one
    byte must hold '\0'. WPARAM is not limited to this text capacity; the two
    WM_COMMAND fields used here are each 16 bits, so a control ID must fit in 16 bits.

    message loop: the repeated GetMessageA, TranslateMessage, and DispatchMessageA
    calls that wait for Windows messages and send them to the window callback.
    WNDCLASSA is a C structure describing a window type; it is not a C++ class. */

/* windows.h supplies the Win32 API declarations used by this GUI. The C headers
    provide bool, formatted output, number parsing, and string functions. */

/* A control ID is an integer assigned to a child control at creation. Windows
    sends it to the parent in WM_COMMAND, a message used to report menu, control,
    and accelerator actions. WPARAM is a pointer-sized unsigned integer type, not
    a floating-point type. Its size follows the build: 32 bits in a 32-bit build
    and 64 bits in this x86_64 build. For a button's WM_COMMAND notification,
    LOWORD(wparam) extracts the low 16-bit control ID and HIWORD(wparam) extracts
    the next 16-bit notification code. LPARAM is the message's second parameter;
    for a control notification it carries the child's HWND. Each ID must fit in
    the 16-bit field used by WM_COMMAND. Digit IDs are 0-9; named IDs identify
    the other controls. */
#define ID_CLEAR 100
#define ID_DELETE 101
#define ID_DECIMAL 102
#define ID_SIGN 103
#define ID_ADD 104
#define ID_SUBTRACT 105
#define ID_MULTIPLY 106
#define ID_DIVIDE 107
#define ID_EQUALS 108
#define ID_DISPLAY 109

/* HWND is an opaque, pointer-sized Win32 handle that identifies a window or
    control. HFONT is a handle to a font object managed by GDI, Windows' Graphics
    Device Interface, which provides drawing resources such as fonts. Handles are
    not floats or ordinary numbers to calculate with; pass them to the Windows
    functions that understand them. These file-scope variables are shared by the
    callback and helper functions. static gives these names internal linkage, so
    other source files cannot refer to them by name. C does not make them class
    members or object properties. */
static HWND display;
static HFONT display_font;
static double stored_value;
static char pending_operator;
/* This flag tracks the input phase. true means the next digit replaces the
    display and begins an operand. An operator sets it before the second operand;
    entering a digit clears it so more digits append to that operand. */
static bool start_new_number = true;

/* This helper contains arithmetic only, separate from Windows event handling.
   C returns one value directly, so result is an output parameter: double * means
   it points to a double, and *result accesses the value at that address. The
   caller passes an address with &, letting this helper write its answer there.
   The bool return separately reports success or failure. */
static bool calculate(double left, char operator, double right, double *result)
{
    /* If answer is a double, &answer means its address. Passing it here lets
       *result = 6 update answer in the caller's function. */
    switch (operator) {
        case '+':
            *result = left + right;
            return true;
        case '-':
            *result = left - right;
            return true;
        case '*':
            *result = left * right;
            return true;
        case '/':
            if (right == 0.0) {
                                /* Reject a zero divisor before division. Returning false lets
                                     the caller show an error rather than use an infinite result. */
                return false;
            }
            *result = left / right;
            return true;
        default:
            return false;
    }
}

static void show_number(double value)
{
    char text[64];
    /* The edit control displays text, so snprintf converts the double into the
       char array first. text has 64 bytes; snprintf reserves one for the '\0'
       terminator, leaving at most 63 bytes of text. %.12g prints up to 12
       significant digits. SetWindowTextA copies the resulting text to the UI. */
    snprintf(text, sizeof text, "%.12g", value);
    SetWindowTextA(display, text);
}

/* A C string is a char array ending with '\0'. This buffer holds 64 bytes, so
    it can store at most 63 text bytes plus the terminator. GetWindowTextA's size
    argument includes space for that terminator. strtod parses the text as double;
    NULL means this code does not need the pointer to the first unparsed character. */
static double displayed_number(void)
{
    char text[64];
    GetWindowTextA(display, text, sizeof text);
    return strtod(text, NULL);
}

static void press_digit(char digit)
{
    /* Every digit button routes here. Centralizing input rules makes starting a
       new operand, replacing zero, and appending a digit consistent. The buffer
       check below prevents writing past its 63-byte text capacity. */
    char text[64];
    GetWindowTextA(display, text, sizeof text);

    if (start_new_number || strcmp(text, "Error") == 0) {
        text[0] = digit;
        text[1] = '\0';
    } else if (strcmp(text, "0") == 0) {
        text[0] = digit;
        text[1] = '\0';
    } else if (strcmp(text, "-0") == 0) {
        text[1] = digit;
        text[2] = '\0';
    } else {
        size_t length = strlen(text);
        if (length + 1 < sizeof text) {
            text[length] = digit;
            text[length + 1] = '\0';
        }
    }

    SetWindowTextA(display, text);
    start_new_number = false;
}

static void press_decimal(void)
{
    /* Permit one decimal point per operand. Starting with "0." produces text
       strtod can parse even when the user presses the decimal button first. */
    char text[64];
    GetWindowTextA(display, text, sizeof text);

    if (start_new_number || strcmp(text, "Error") == 0) {
        SetWindowTextA(display, "0.");
    } else if (strchr(text, '.') == NULL) {
        size_t length = strlen(text);
        if (length + 1 < sizeof text) {
            text[length] = '.';
            text[length + 1] = '\0';
            SetWindowTextA(display, text);
        }
    }

    start_new_number = false;
}

static void press_operator(char operator)
{
    /* Save this operator until another operand is entered. If a previous
       operator and its right operand are already available, calculate them
       first. Chained operations therefore run left to right in this simple UI. */
    double current_value = displayed_number();

    if (pending_operator != '\0' && !start_new_number) {
        if (!calculate(stored_value, pending_operator, current_value,
                       &current_value)) {
            SetWindowTextA(display, "Error");
            pending_operator = '\0';
            start_new_number = true;
            return;
        }
        show_number(current_value);
    }

    stored_value = current_value;
    pending_operator = operator;
    start_new_number = true;
}

static void press_equals(void)
{
    /* Use the saved left operand and operator with the display as the right
       operand. Clear the saved operator after this attempt so it is not reused. */
    double result;

    if (pending_operator == '\0') {
        return;
    }

    if (!calculate(stored_value, pending_operator, displayed_number(), &result)) {
        SetWindowTextA(display, "Error");
    } else {
        show_number(result);
    }

    pending_operator = '\0';
    start_new_number = true;
}

static void press_sign(void)
{
    /* Toggle the leading minus in the display text. memmove supports overlapping
       source and destination ranges, which occur when shifting the string. */
    char text[64];

    if (start_new_number) {
        SetWindowTextA(display, "0");
        start_new_number = false;
    }

    GetWindowTextA(display, text, sizeof text);

    if (strcmp(text, "Error") == 0) {
        return;
    }

    if (text[0] == '-') {
        memmove(text, text + 1, strlen(text));
    } else if (strlen(text) + 1 < sizeof text) {
        memmove(text + 1, text, strlen(text) + 1);
        text[0] = '-';
    }

    SetWindowTextA(display, text);
    start_new_number = false;
}

static void create_button(HWND window, const char *label, int id,
                          int column, int row)
{
    /* This wrapper keeps the repeated CreateWindowA arguments in one place.
       The layout only has to provide each button's label, ID, and grid position. */
    const int button_width = 64;
    const int button_height = 52;
    const int gap = 6;
    const int left = 14 + column * (button_width + gap);
    const int top = 84 + row * (button_height + gap);

    /* Win32 control positions are measured in pixels. Convert row and column to
       pixel offsets while using the same button size and spacing throughout. */
    CreateWindowA("BUTTON", label, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  left, top, button_width, button_height, window,
                  (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
}

static void create_calculator_controls(HWND window)
{
        /* Buttons and the display are child windows. Passing the main window as
             parent groups them inside it and lets Windows send their notifications
             to the parent's window procedure. */
    display = CreateWindowA("EDIT", "0",
                            WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_READONLY,
                            14, 16, 274, 54, window,
                            (HMENU)(INT_PTR)ID_DISPLAY,
                            GetModuleHandleA(NULL), NULL);
    display_font = CreateFontA(30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    SendMessageA(display, WM_SETFONT, (WPARAM)display_font, TRUE);

    /* Keep layout data together: label controls visible text, ID identifies the
       control in WM_COMMAND, and row/column determine its screen position. */
    create_button(window, "C", ID_CLEAR, 0, 0);
    create_button(window, "DEL", ID_DELETE, 1, 0);
    create_button(window, "/", ID_DIVIDE, 2, 0);
    create_button(window, "*", ID_MULTIPLY, 3, 0);

    create_button(window, "7", 7, 0, 1);
    create_button(window, "8", 8, 1, 1);
    create_button(window, "9", 9, 2, 1);
    create_button(window, "-", ID_SUBTRACT, 3, 1);

    create_button(window, "4", 4, 0, 2);
    create_button(window, "5", 5, 1, 2);
    create_button(window, "6", 6, 2, 2);
    create_button(window, "+", ID_ADD, 3, 2);

    create_button(window, "1", 1, 0, 3);
    create_button(window, "2", 2, 1, 3);
    create_button(window, "3", 3, 2, 3);
    create_button(window, "=", ID_EQUALS, 3, 3);

    create_button(window, "+/-", ID_SIGN, 0, 4);
    create_button(window, "0", 0, 1, 4);
    create_button(window, ".", ID_DECIMAL, 2, 4);
}

static LRESULT CALLBACK window_procedure(HWND window, UINT message,
                                         WPARAM wparam, LPARAM lparam)
{
    /* Windows calls this callback when a message targets this window. HWND is
       the window handle, UINT is an unsigned message number, WPARAM and LPARAM
       carry message-specific values, and LRESULT is the callback's return type. */
    (void)lparam;

    switch (message) {
        case WM_CREATE:
            /* The system sends WM_CREATE during main-window initialization. The
               parent HWND now exists, so create its child controls. */
            create_calculator_controls(window);
            return 0;

        case WM_COMMAND: {
                /* WM_COMMAND reports actions from menus and child controls. For a
                    child-control notification, LOWORD(wparam) extracts its ID and
                    HIWORD(wparam) extracts its notification code; lparam carries the
                    child HWND. BN_CLICKED means a button was clicked, so this check
                    ignores other notifications before routing the action. */
            if (HIWORD(wparam) != BN_CLICKED) {
                return 0;
            }

                /* Match the ID assigned at creation to the operation to perform.
                    The digit IDs map directly to characters '0' through '9'. */
            int id = LOWORD(wparam);

            if (id >= 0 && id <= 9) {
                press_digit((char)('0' + id));
            } else {
                switch (id) {
                    case ID_CLEAR:
                        stored_value = 0.0;
                        pending_operator = '\0';
                        start_new_number = true;
                        SetWindowTextA(display, "0");
                        break;
                    case ID_DELETE: {
                        char text[64];
                        GetWindowTextA(display, text, sizeof text);
                        size_t length = strlen(text);
                        if (!start_new_number && length > 1) {
                            text[length - 1] = '\0';
                            if (strcmp(text, "-") == 0) {
                                strcpy(text, "0");
                            }
                            SetWindowTextA(display, text);
                        } else if (!start_new_number) {
                            SetWindowTextA(display, "0");
                        }
                        break;
                    }
                    case ID_DECIMAL:
                        press_decimal();
                        break;
                    case ID_SIGN:
                        press_sign();
                        break;
                    case ID_ADD:
                        press_operator('+');
                        break;
                    case ID_SUBTRACT:
                        press_operator('-');
                        break;
                    case ID_MULTIPLY:
                        press_operator('*');
                        break;
                    case ID_DIVIDE:
                        press_operator('/');
                        break;
                    case ID_EQUALS:
                        press_equals();
                        break;
                    default:
                        break;
                }
            }
            return 0;
        }

        case WM_DESTROY:
            /* WM_DESTROY signals that the main window is being removed.
               DeleteObject releases the GDI font object. PostQuitMessage queues
               WM_QUIT, which makes the message loop exit. */
            if (display_font != NULL) {
                DeleteObject(display_font);
            }
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcA(window, message, wparam, lparam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance,
                   LPSTR command_line, int show_command)
{
    (void)previous_instance;
    (void)command_line;

        /* WinMain is the entry point for a Windows-subsystem executable. WNDCLASSA
             is a C structure describing a window type, not an object-oriented class.
             RegisterClassA gives Windows the callback and default settings; the name
             is then used by CreateWindowExA to create the actual main window. */
    const char class_name[] = "LearningCalculatorWindow";
    WNDCLASSA window_class = {0};
    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassA(&window_class)) {
        return 1;
    }

    HWND window = CreateWindowExA(
        0, class_name, "C Calculator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 316, 430,
        NULL, NULL, instance, NULL);
    if (window == NULL) {
        return 1;
    }

    ShowWindow(window, show_command);
    UpdateWindow(window);

        /* This is the GUI message loop. GetMessageA waits for a queued message and
             returns zero after WM_QUIT. TranslateMessage creates character messages
             from keyboard messages when applicable. DispatchMessageA calls the
             registered window_procedure callback to handle each event. */
    MSG message;
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return (int)message.wParam;
}