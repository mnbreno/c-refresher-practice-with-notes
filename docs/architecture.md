# C Calculator Architecture

This diagram shows the Win32 GUI, the shared arithmetic core, and the diagnostic paths. The implementation is procedural C: `WNDCLASSA` describes a window type, and Windows calls a registered callback; these are not user-defined C++ classes.

```mermaid
flowchart LR
    User[User]
    Windows[Windows OS]
    Controls["Child controls<br/>buttons and display"]
    Queue["Thread message queue"]
    Loop["GetMessageA / TranslateMessage<br/>/ DispatchMessageA"]
    Proc["window_procedure(HWND, UINT,<br/>WPARAM, LPARAM)"]
    IdRoute["WM_COMMAND routing<br/>control ID + notification"]
    Input["press_digit / press_operator<br/>press_equals / press_decimal"]
    DisplayText["displayed_number / show_number"]
    Core["calculator_core.h<br/>calculate(...)<br/>Win32-independent"]
    State["Calculator state<br/>stored_value<br/>pending_operator"]
    Error["show_error()<br/>reset pending state"]
    Create["WinMain / WM_CREATE<br/>create controls and font"]
    Cleanup["WM_DESTROY<br/>DeleteObject(HFONT)"]
    Filter["Unhandled exception filter"]
    DbgHelp["DbgHelp<br/>MiniDumpWriteDump"]
    Dump[("calculator-crash.dmp")]
    Log[("calculator-crash.log")]

    User -->|"click"| Controls
    Controls -->|"WM_COMMAND notification"| Queue
    Windows --> Queue
    Queue --> Loop
    Loop --> Proc
    Proc --> IdRoute
    IdRoute --> Input
    Input <--> State
    Input --> DisplayText
    DisplayText --> Core
    Core -->|"success: numeric result"| DisplayText
    Core -.->|"failure"| Error
    Error --> Controls
    Windows -->|"startup callback"| Create
    Create -->|"CreateWindowA"| Controls
    Windows -->|"close callback"| Cleanup
    Windows -.->|"unhandled exception"| Filter
    Filter --> DbgHelp
    DbgHelp --> Dump
    Filter --> Log
```

## Button-click sequence

```mermaid
sequenceDiagram
    actor User
    participant Button as Win32 button
    participant OS as Windows message system
    participant MessagePump as Windows message pump
    participant WndProc as window_procedure
    participant Core as calculate in calculator_core.h
    participant Display as Edit control

    User->>Button: Click an operator or digit
    Button->>OS: Send WM_COMMAND with control ID
    OS->>MessagePump: Queue message
    MessagePump->>WndProc: DispatchMessageA invokes callback
    WndProc->>WndProc: Read ID with LOWORD(WPARAM)
    WndProc->>Core: Calculate operands when complete
    alt calculation succeeds
        Core-->>WndProc: Write result through output pointer
        WndProc->>Display: Format and set result text
    else invalid operation or result
        Core-->>WndProc: Return false
        WndProc->>Display: Show Error and clear pending state
    end
    Display-->>User: Windows repaints the control
```

`calculator_core.h` is deliberately separate so both the GUI and `tests/test_calculator.c` compile and execute the same arithmetic function. The GUI test is headless and does not initialize Win32 windows. The crash filter writes a minidump and a text record on unexpected process exceptions; it is diagnostic output, not application recovery.
