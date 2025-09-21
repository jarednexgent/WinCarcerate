# CellBlock32

> 🚨 **DISCLAIMER:**  
> This tool is provided **solely** for educational, research, and defensive-security purposes.  
> Do **not** run CellBlock32 against any system you do not own or have **explicit** permission to test.  
> Unauthorized or malicious use is **illegal** and the authors disclaim any liability for damages or legal consequences arising from misuse.

[![cellblock32.png](https://i.postimg.cc/tTT6h7G0/cellblock32.png)](https://postimg.cc/TyzhxdPQ)

CellBlock32 is a ransomware simulator for Windows. Built without the C runtime (CRT), its binary size stays under 15KB while retaining functionality like multithreading, debugger evasion, and self-deletion.

## Build

Open ___Developer Command Prompt for VS 2022___ and run `Builder.bat`. The script will:

- Ask what to build (**Locker** or **Decryptor**).
- Optionally set the **scan root** (default is defined in `config.h`).
- Toggle features: **console stdout** (also switches Console/Windows subsystem), **debugger evasion**, and **self-delete**.
- Compile using `msbuild`. Output is `x64\Release\CellBlock32.exe`.

```
C:\Users\Maldev\source\repos\CellBlock32>Builder.bat

=== CellBlock32 Build (Release|x64) ===

Build which? (Locker/Decryptor): Locker
Root directory? (blank = keep default in config.h): C:\Users\Public\Pictures
Enable console stdout? (Y/N): y
Enable debugger evasion? (Y/N): y
Enable self deletion? (Y/N): y

Using:
  Root     = C:\Users\Public\Pictures
  Override = CellBlock32\override.h
#pragma once
#undef CONFIG_ROOT_DIR
#define CONFIG_ROOT_DIR L"C:\\Users\\Public\\Pictures", NULL
  CL       =  /DCONSOLE_STDOUT /DDBG_EVASION /DSELF_DESTRUCT /DSIMULATE_RANSOM
  SubSystem= Console

Build started 9/8/2025 2:41:15 AM.
```
> **Note**: `override.h` is regenerated on each run; your committed `config.h` isn't modified.


## Usage

No command-line arguments required. `CellBlock32.exe` will begin encrypting or decrypting files recursively, starting from the user-defined root directory. 

[![cellblock32-demo.gif](https://i.postimg.cc/MpynCVmb/cellblock32-demo.gif)](https://postimg.cc/Wd1prDKh)

