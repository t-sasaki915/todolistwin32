#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

typedef struct
{
    BOOL topMostEnabled;
    HFONT customTextListFont;
} ConfigData;

BOOL LoadConfigData(ConfigData *data);

#endif

