#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

typedef struct
{
    BOOL topMostEnabled;
    HFONT customTextListFont;
} ConfigData;

BOOL InitialiseConfig(HINSTANCE hInstance, HWND mainHWND);

BOOL LoadConfigData(ConfigData *data);

BOOL SetTopMostEnabledConfig(BOOL enabled);

BOOL SetCustomTextListFontConfig(HFONT font);

void FreeConfigFilePath(void);

#endif

