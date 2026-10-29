# C Refresher Practice With Notes

A native Windows calculator written in C with the Win32 API. The source comments explain C pointers and strings, Windows handles, control notifications, and the GUI message loop.

See the [architecture diagrams](docs/architecture.md) for the Win32 component and message-flow views.

## Versioning

The current version is recorded in `VERSION`. Update it in every commit so each commit has its own version: increment the patch number for fixes and documentation, the minor number for backward-compatible features, and the major number for incompatible changes. Published Git tags and releases use the same number with a `v` prefix.

## Build on Windows

Install GCC with the MinGW-w64 tools, including `windres`. From this folder, compile the icon resource and GUI:

```powershell
windres -J rc -O coff calculator.rc calculator-resource.o
gcc -std=c11 -Wall -Wextra -Wpedantic -mwindows -static '-Wl,--nxcompat' '-Wl,--strip-all' calculator.c calculator-resource.o -o calculator-gui.exe
```

Then launch it:

```powershell
.\calculator-gui.exe
```

See the [GUI cheat sheet](GUI-CHEATSHEET.md) for button actions.

The program uses Win32 APIs and currently builds for Windows. The `icons` folder also includes PNG and ICNS versions for future Linux or macOS ports.

## Error handling

The display shows `Error` for empty or malformed numeric text, values outside the supported `double` range, division by zero, or arithmetic that overflows to infinity. The pending operation is cleared, so entering a digit or pressing `C` starts a fresh calculation. Startup also reports failures to register or create the window, and the program reports a message-loop failure with a Windows dialog.

For an unhandled Windows exception, the registered exception filter makes a best-effort attempt to create `calculator-crash.dmp` and append details to `calculator-crash.log` beside the executable. Microsoft's DbgHelp library is loaded from System32 at startup and resolved at runtime; linking with `-ldbghelp` is no longer required. The log records the time, exception code/address, and whether a `MiniDumpNormal` dump was written. Dump creation can fail, and the filter does not recover the application. Dumps may contain private process memory; keep them local and inspect them before sharing. Both generated files are excluded from Git.

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
.\scripts\recover.ps1 -Version v1.2.8 -Destination ..\c-calculator-recovered
```

The script refuses to overwrite an existing destination, clones the release tag, then rebuilds the GUI and its icon resource. Crash logs and minidumps are diagnostics, not backups; the minidump can contain sensitive process memory and should be handled privately.

## Executable and VirusTotal report

The unsigned MinGW-w64 Windows executable is included as `calculator-gui.exe` (45,568 bytes). Its SHA-256 is `b9aeec49fec212a73dcb9b3aced5a67a486c1cf3a1142924f5898b7d7540f030`, also recorded in [`SHA256SUMS.txt`](SHA256SUMS.txt). The [VirusTotal report](https://www.virustotal.com/gui/file/b9aeec49fec212a73dcb9b3aced5a67a486c1cf3a1142924f5898b7d7540f030) showed **0 of 69 detections**, with its latest analysis dated 2026-10-29 at 17:20:38 EDT. Bkav Pro and Webroot both reported Undetected; ClamAV and Rising timed out, and four mobile engines could not process the file type. These results apply to this exact executable and are not a security certification.

Release v1.2.8 preserves the exact scanned executable, whose embedded file/product version and manifest identity remain 1.2.7. The source includes static runtime linking, DEP compatibility, version resources, an application manifest, and runtime DbgHelp loading. Rebuilds can have different hashes because of toolchain versions and build timestamps; the report above does not cover rebuilt binaries. The previous executable's detections do not apply to this new hash, and this report does not establish a vendor reclassification of the old file. Crash dumps and logs are private diagnostics and are not release assets.

## Topics practiced

- C functions, pointers, output parameters, arrays, and null-terminated strings
- Windows handles, controls, control IDs, and resources
- `WM_COMMAND` notifications and message parameter fields
- Window callbacks, message loops, and GDI font cleanup
- Resource compilation and linking with MinGW-w64
