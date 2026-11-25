#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

typedef struct
{
    BOOL topMostEnabled;
    HFONT customTextListFont;
} ConfigData;

void InitialiseConfig(HINSTANCE hInstance, HWND mainHWND);

BOOL LoadConfigData(ConfigData *data);

#endif

