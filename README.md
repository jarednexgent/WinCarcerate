# WinCarcerate

> 🚨 **DISCLAIMER:**  
> This tool is provided **solely** for educational, research, and defensive-security purposes.  
> Do **not** run WinCarcerate against any system you do not own or have **explicit** permission to test.  
> Unauthorized or malicious use is **illegal** and the authors disclaim any liability for damages or legal consequences arising from misuse.

[![wincarcerate.png](https://i.postimg.cc/L55XRRBx/wincarcerate.png)](https://postimg.cc/CnyYH328)

WinCarcerate is a simulated ransomware for Windows. Built without the C runtime (CRT), its binary size stays under 15KB while retaining functionality like multithreading, debugger evasion, and self-deletion.

## Build

In the **Developer Command Prompt for VS 2022**, run `Build.bat`. After the prompts, `msbuild` produces `x64\Release\Wincarcerate.exe`.

```
C:\Tools\WinCarcerate>Build.bat

=== WinCarcerate Build (Release|x64) ===

Build which? (Locker/Decryptor): Locker
Subsystem? (Console/Windows): Console
Enable debugger evasion? (Y/N): Y
Enable self deletion? (Y/N): Y
Start from which directory? (Leave blank for current directory): C:\Temp

Using:
  Root     = C:\Temp
  Override = WinCarcerate\override.h
#pragma once
#undef CONFIG_ROOT_DIR
#define CONFIG_ROOT_DIR L"C:\\Temp", NULL
  CL       =  /DCONSOLE_STDOUT /DDBG_EVASION /DSELF_DESTRUCT /DSIMULATE_RANSOM
  SubSystem= Console

Build started 10/12/2025 5:09:22 PM.
```
> **Note**: `override.h` is regenerated on each run.


## Usage

No command-line arguments required. Upon execution, `WinCarcerate.exe` will begin encrypting or decrypting files recursively starting from directory set at build-time.

[![wincarcerate-demo.gif](https://i.postimg.cc/4y6c7Xhf/wincarcerate-demo.gif)](https://postimg.cc/Wq4zxLnC)