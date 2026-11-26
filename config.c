#include "config.h"

#include <windows.h>
#include <shlwapi.h>

#include "simple_ini.h"
#include "resource.h"
#include "util.h"

#define CONFIG_FILE_NAME_LENGTH 10
#define CONFIG_FILE_NAME L"config.ini"

typedef struct
{
    BOOL topMostEnabled;
    wchar_t *fontName;
    LONG fontSize;
} RawConfigData;

static HINSTANCE g_hInstance = NULL;
static HWND g_mainHWND = NULL;

static RawConfigData g_defConfig = {
    TRUE,
    L"Tahoma",
    8
};

void InitialiseConfig(HINSTANCE hInstance, HWND mainHWND)
{
    g_hInstance = hInstance;
    g_mainHWND = mainHWND;
}

int PointSizeToHeight(int pointSize)
{
    HDC hdc = GetDC(g_mainHWND);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

    ReleaseDC(g_mainHWND, hdc);

    return -MulDiv(pointSize, dpiY, 72);
}

BOOL CreateConfigDataFromRawConfig(RawConfigData *rawData, ConfigData *data)
{
    HFONT customTextListFont = CreateFontW(
        PointSizeToHeight(rawData->fontSize),
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        rawData->fontName
    );
    if (customTextListFont == NULL)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_FAILED_TO_CREATE_FONT_MSG,
            GetLastError()
        );

        return FALSE;
    }

    data->topMostEnabled     = rawData->topMostEnabled;
    data->customTextListFont = customTextListFont;

    return TRUE;
}

wchar_t *GetConfigFilePath(void)
{
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(NULL, modulePath, MAX_PATH);

    int modulePathLength = wcslen(modulePath);

    int lastBackslashIndex = 0;
    for (int i = modulePathLength - 1; i >= 0; i --)
    {
        if (modulePath[i] == L'\\')
        {
            lastBackslashIndex = i;
            break;
        }
    }

    int parentPathLength = lastBackslashIndex + 1;
    int configPathLength = parentPathLength + CONFIG_FILE_NAME_LENGTH;

    wchar_t *configFilePath = calloc(configPathLength + 1, sizeof(wchar_t));
    if (configFilePath == NULL)
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_MEMORY_ERROR_TITLE,
            IDS_FAILED_TO_ALLOCATE_MEMORY_MSG
        );

        return NULL;
    }

    memcpy(
        configFilePath,
        modulePath,
        parentPathLength * sizeof(wchar_t)
    );

    memcpy(
        &(configFilePath[lastBackslashIndex + 1]),
        CONFIG_FILE_NAME,
        CONFIG_FILE_NAME_LENGTH * sizeof(wchar_t)
    );

    configFilePath[configPathLength] = L'\0';

    return configFilePath;
}

BOOL LoadConfigData(ConfigData *data)
{
    wchar_t *configFilePath = GetConfigFilePath();
    if (configFilePath == NULL)
    {
        return FALSE;
    }

    if (!PathFileExistsW(configFilePath))
    {
        free(configFilePath);
        configFilePath = NULL;

        if (!CreateConfigDataFromRawConfig(&g_defConfig, data))
        {
            return FALSE;
        }

        return TRUE;
    }

    IniFile iniFile;
    if (!ReadIniFile(configFilePath, &iniFile))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_FAILED_TO_READ_CONFIG_FILE_MSG,
            GetLastIniError()
        );

        free(configFilePath);
        configFilePath = NULL;

        return FALSE;
    }

    free(configFilePath);
    configFilePath = NULL;

    wchar_t *configCurrentlyReading = NULL;

    BOOL topMostEnabled;
    if (!GetBoolFromIni(&iniFile, L"General", L"AlwaysOnTop", &topMostEnabled))
    {
        if (!SubstituteBoolDefault(&topMostEnabled, g_defConfig.topMostEnabled, TRUE))
        {
            configCurrentlyReading = L"General.AlwaysOnTop";

            goto ShowErrorAndFail;
        }
    }

    wchar_t *fontName;
    if (!GetStringFromIni(&iniFile, L"Font", L"FontName", &fontName))
    {
        if (!SubstituteStringDefault(&fontName, g_defConfig.fontName, TRUE))
        {
            configCurrentlyReading = L"Font.FontName";

            goto ShowErrorAndFail;
        }
    }

    LONG fontSize;
    if (!GetLongFromIni(&iniFile, L"Font", L"FontSize", &fontSize))
    {
        if (!SubstituteLongDefault(&fontSize, g_defConfig.fontSize, TRUE))
        {
            configCurrentlyReading = L"Font.FontSize";

            goto ShowErrorAndFail;
        }
    }

    RawConfigData rawConfig  = {0};
    rawConfig.topMostEnabled = topMostEnabled;
    rawConfig.fontName       = fontName;
    rawConfig.fontSize       = fontSize;

    if (!CreateConfigDataFromRawConfig(&rawConfig, data))
    {
        goto Fail;
    }

    FreeIniFile(&iniFile);

    return TRUE;

ShowErrorAndFail:
    MessageBoxUsingResourceString(
        g_hInstance,
        g_mainHWND,
        MB_ICONERROR | MB_OK,
        IDS_CONFIG_ERROR_TITLE,
        IDS_INI_ERROR_MSG,
        configCurrentlyReading,
        GetLastIniError()
    );

Fail:
    FreeIniFile(&iniFile);

    return FALSE;
}

