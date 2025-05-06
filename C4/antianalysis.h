#pragma once
#include <Windows.h>

#ifndef ANTI_ANALYSIS_H
#define ANTI_ANALYSIS_H

BOOL HideConsoleWindow();

BOOL IsBeingDebugged();

BOOL DeleteSelf();
#endif