# C Refresher Practice With Notes

A native Windows calculator written in C with the Win32 API. The source comments explain C pointers and strings, Windows handles, control notifications, and the GUI message loop.

## Build on Windows

Install GCC with the MinGW-w64 tools, including `windres`. From this folder, compile the icon resource and GUI:

```powershell
windres calculator.rc calculator-resource.o
gcc -std=c11 -Wall -Wextra -Wpedantic -mwindows calculator.c calculator-resource.o -o calculator-gui.exe
```

Then launch it:

```powershell
.\calculator-gui.exe
```

The program uses Win32 APIs and currently builds for Windows. The `icons` folder also includes PNG and ICNS versions for future Linux or macOS ports.

## Topics practiced

- C functions, pointers, output parameters, arrays, and null-terminated strings
- Windows handles, controls, control IDs, and resources
- `WM_COMMAND` notifications and message parameter fields
- Window callbacks, message loops, and GDI font cleanup
- Resource compilation and linking with MinGW-w64