# C Refresher Practice With Notes

A native Windows calculator written in C with the Win32 API. The source comments explain C pointers and strings, Windows handles, control notifications, and the GUI message loop.

## Versioning

The current version is recorded in `VERSION`. Update it in every commit so each commit has its own version: increment the patch number for fixes and documentation, the minor number for backward-compatible features, and the major number for incompatible changes. Published Git tags and releases use the same number with a `v` prefix.

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

## Error handling

The display shows `Error` for empty or malformed numeric text, values outside the supported `double` range, division by zero, or arithmetic that overflows to infinity. The pending operation is cleared, so entering a digit or pressing `C` starts a fresh calculation. Startup also reports failures to register or create the window, and the program reports a message-loop failure with a Windows dialog.

## Topics practiced

- C functions, pointers, output parameters, arrays, and null-terminated strings
- Windows handles, controls, control IDs, and resources
- `WM_COMMAND` notifications and message parameter fields
- Window callbacks, message loops, and GDI font cleanup
- Resource compilation and linking with MinGW-w64