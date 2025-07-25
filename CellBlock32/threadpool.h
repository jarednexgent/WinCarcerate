#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <Windows.h>


#define MAX_THREADS     8     


typedef struct _TP_STATE {
    TP_CALLBACK_ENVIRON  Env;
    PTP_CLEANUP_GROUP    Cleanup;
    PTP_POOL             Pool;
} TP_STATE, * PTP_STATE;


typedef struct _THREAD_PARAMS {
    WCHAR   Path[MAX_PATH];
    BOOL    Encrypt;
} THREAD_PARAMS, * PTHREAD_PARAMS;



VOID CALLBACK FileWorkerTP(PTP_CALLBACK_INSTANCE Instance, PVOID pContext, PTP_WORK Work);
BOOL ParallelProcessDirectory(LPCWSTR rootDir, BOOL encryptMode);


#endif // THREADPOOL_H