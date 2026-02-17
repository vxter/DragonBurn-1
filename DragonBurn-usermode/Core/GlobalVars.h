#pragma once
#include "../Game/Game.h"
#include "Globals.h"

class globalvars
{
public:
    DWORD64 address = 0;

    float g_fRealTime = 0.0f;
    int g_iFrameCount = 0;
    int g_iMaxClients = 0;
    float g_fIntervalPerTick = 0.0f;
    float g_fCurrentTime = 0.0f;
    float g_fCurrentTime2 = 0.0f;
    int g_iTickCount = 0;
    float g_fIntervalPerTick2 = 0.0f;
    void* g_vCurrentNetchan = nullptr;
    char* g_cCurrentMap = nullptr;
    char* g_cCurrentMapName = nullptr;

    bool UpdateGlobalvars();
    bool GetRealTime();
    bool GetFrameCount();
    bool GetMaxClients();
    bool GetIntervalPerTick();
    bool GetcurrentTime();
    bool GetcurrentTime2();
    bool GetTickCount();
    bool GetIntervalPerTick2();
    bool GetCurrentNetchan();
    bool GetCurrentMap();
    bool GetCurrentMapName();
};

inline std::unique_ptr<globalvars> g_globalVars = nullptr;