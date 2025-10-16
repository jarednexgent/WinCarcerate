# WinCarcerate

> 🚨 **DISCLAIMER:**  
> This software is provided **solely** for research, educational, and defensive-security purposes.  
> Do **not** run WinCarcerate against any system you do not own or have **explicit** permission to test.  
> Unauthorized or malicious use is **illegal** and the authors disclaim any liability for damages or legal consequences arising from misuse.

[![wincarcerate.png](https://i.postimg.cc/JnnD9zXz/wincarcerate.png)](https://postimg.cc/p9wLjxkw)

**WinCarcerate** is a compact (~15 KB) Windows ransomware sample demonstrating Thread Pools, ChaCha20 symmetric encryption, and anti-analysis techniques. For training and research only.

## Build 

Open the **Developer Command Prompt for Visual Studio 2022**, run `Build.bat`, and follow the interactive prompts.

```
C:\Tools\WinCarcerate>Build.bat

=== WinCarcerate Build (Release|x64) ===

Toolchain: [M]SVC or [L]LVM? M
Executable: [L]ocker or [D]ecryptor? L
SubSystem: [C]onsole or [W]indows? C
Enable Debugger Evasion: [Y]es or [N]o? Y
Enable Self Deletion: [Y]es or [N]o? Y
Root directory for encryption/decryption [Enter = current]: C:\Temp

Build started 10/12/2025 5:09:22 PM.
```

Each run of `Build.bat` produces a single output binary at `x64\Release\WinCarcerate.exe`. If you need both Locker and Decryptor variants for a lab, run the build twice and preserve the first output before rebuilding so it is not overwritten.

Building with the LLVM toolchain requires [MSBuild support for LLVM (clang-cl) toolset](https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170). Compiling as a **Console** application prints encryption progress and total runtime to stdout; the **Windows** subsystem produces a smaller, quieter binary (no console output).

## Usage

No command-line arguments are required. On launch, `WinCarcerate.exe` recursively encrypts or decrypts files starting at the build-time root directory. 

[![wincarcerate-demo.gif](https://i.postimg.cc/j5gPYRn3/wincarcerate-demo.gif)](https://postimg.cc/qhKz8VL2)

