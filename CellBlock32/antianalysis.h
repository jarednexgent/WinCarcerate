// antianalysis.h
#ifndef ANTIANALYSIS_H
#define ANTIANALYSIS_H


#include <Windows.h>


#define DELETE_HANDLE(H)								\
	if (H != NULL && H != INVALID_HANDLE_VALUE){		\
		CloseHandle(H);									\
		H = NULL;										\
	}


BOOL _IsDebuggerPresent(void);
BOOL DeleteSelf(void);

#endif // ANTI_ANALYSIS_H



