# C Refresher Practice With Notes

A native Windows calculator written in C with the Win32 API. The source comments explain C pointers and strings, Windows handles, control notifications, and the GUI message loop.

See the [architecture diagrams](docs/architecture.md) for the Win32 component and message-flow views.

## Versioning

The current version is recorded in `VERSION`. Update it in every commit so each commit has its own version: increment the patch number for fixes and documentation, the minor number for backward-compatible features, and the major number for incompatible changes. Published Git tags and releases use the same number with a `v` prefix.

## Build on Windows

Install GCC with the MinGW-w64 tools, including `windres`. From this folder, compile the icon resource and GUI:

```powershell
windres calculator.rc calculator-resource.o
gcc -std=c11 -Wall -Wextra -Wpedantic -mwindows calculator.c calculator-resource.o -ldbghelp -o calculator-gui.exe
```

Then launch it:

```powershell
.\calculator-gui.exe
```

See the [GUI cheat sheet](GUI-CHEATSHEET.md) for button actions.

The program uses Win32 APIs and currently builds for Windows. The `icons` folder also includes PNG and ICNS versions for future Linux or macOS ports.

## Error handling

The display shows `Error` for empty or malformed numeric text, values outside the supported `double` range, division by zero, or arithmetic that overflows to infinity. The pending operation is cleared, so entering a digit or pressing `C` starts a fresh calculation. Startup also reports failures to register or create the window, and the program reports a message-loop failure with a Windows dialog.

For an unhandled Windows exception, the registered exception filter makes a best-effort attempt to create `calculator-crash.dmp` and append details to `calculator-crash.log` beside the executable. The minidump is created through Microsoft's DbgHelp library, so link with `-ldbghelp`. Dump creation can fail during severe process failures, and recording a dump does not recover the application; Windows still terminates it. A dump may contain sensitive process memory, so keep it private and inspect it before sharing. Both generated files are excluded from Git.

An unhandled Windows exception triggers a best-effort diagnostic write beside the executable: `calculator-crash.log` records the time, exception code/address, and whether a dump was written; `calculator-crash.dmp` is a `MiniDumpNormal` snapshot. The filter cannot recover from the crash, and dump creation can fail during severe failures. A dump can contain process memory and private data, so keep it local and inspect it before sharing. Both generated files are excluded from Git.

## Memory management

The calculator does not dynamically allocate number buffers; local character arrays are automatically released when their functions return, and file-scope state lives until process exit. Windows destroys child controls with the parent window. The program explicitly releases its GDI font with `DeleteObject`. The crash handler closes each dump/log file handle even when writing fails. When adding a new Windows or GDI resource, give it one clear owner and release it on both normal shutdown and partial initialization failure.

## Headless stress tests and measurements

The arithmetic core is in `calculator_core.h`, which has no Win32 dependency. The GUI and test executable include the same function, so tests run without creating a window:

```powershell
.\tests\run_headless.ps1
```

The default run checks 100,000 calls for each operator, plus division-by-zero, unsupported-operator, infinity, and overflow cases. It prints per-operator elapsed time and throughput, and records the measurements in `test-results/c-benchmark.csv`. Set a larger count with `-IterationsPerOperator 1000000`; values below 10,000 are rejected. The timed loop includes result validation and a checksum to prevent the compiler from removing the calculations. Test executables and CSV results are ignored by Git. Timings are machine/load dependent and useful for comparing builds on the same machine, not as universal performance claims.

## Disaster recovery

Restore and rebuild a tagged release into a new, empty folder (PowerShell):

```powershell
.\scripts\recover.ps1 -Version v1.2.2 -Destination ..\c-calculator-recovered
```

The script refuses to overwrite an existing destination, clones the release tag, then rebuilds the GUI and its icon resource. Crash logs and minidumps are diagnostics, not backups; the minidump can contain sensitive process memory and should be handled privately.

## Topics practiced

- C functions, pointers, output parameters, arrays, and null-terminated strings
- Windows handles, controls, control IDs, and resources
- `WM_COMMAND` notifications and message parameter fields
- Window callbacks, message loops, and GDI font cleanup
- Resource compilation and linking with MinGW-w64