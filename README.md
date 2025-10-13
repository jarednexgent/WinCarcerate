# WinCarcerate

> 🚨 **DISCLAIMER:**  
> This tool is provided **solely** for educational, research, and defensive-security purposes.  
> Do **not** run WinCarcerate against any system you do not own or have **explicit** permission to test.  
> Unauthorized or malicious use is **illegal** and the authors disclaim any liability for damages or legal consequences arising from misuse.

[![wincarcerate.png](https://i.postimg.cc/L55XRRBx/wincarcerate.png)](https://postimg.cc/CnyYH328)

WinCarcerate is a simulated ransomware for Windows. Built without the C runtime (CRT), it delivers a ~15 KB optimized binary and remains portable across MSVC and LLVM (Clang-CL) toolchains.

## Build

In the Developer Command Prompt for VS 2022, run `Build.bat`. After answering the prompts, you'll find the executable at `x64\Release\Wincarcerate.exe`.

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
> **Note**: `override.h` is regenerated on each run.


## Usage

No command-line arguments are required. On launch, `WinCarcerate.exe` recursively encrypts or decrypts files starting at the build-time root directory.

[![wincarcerate-demo.gif](https://i.postimg.cc/j5gPYRn3/wincarcerate-demo.gif)](https://postimg.cc/qhKz8VL2)
