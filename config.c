#include "config.h"

#include <windows.h>
#include <shlwapi.h>

#include "simple_ini.h"
#include "resource.h"
#include "util.h"

#define CONFIG_FILE_NAME_LENGTH 10
#define CONFIG_FILE_NAME L"config.ini"

#define GENERAL_SECTION   L"General"
#define ALWAYS_ON_TOP_KEY L"AlwaysOnTop"

#define FONT_SECTION  L"Font"
#define FONT_NAME_KEY L"FontName"
#define FONT_SIZE_KEY L"FontSize"

typedef struct
{
    BOOL topMostEnabled;
    wchar_t *fontName;
    LONG fontSize;
} RawConfigData;

static HINSTANCE g_hInstance = NULL;
static HWND g_mainHWND = NULL;

static wchar_t *g_configFilePath = NULL;

static RawConfigData g_defConfig = {
    TRUE,
    L"Tahoma",
    8
};

BOOL GetConfigFilePath(void)
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

        return FALSE;
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

    if (g_configFilePath != NULL)
    {
        free(g_configFilePath);
        g_configFilePath = NULL;
    }
    g_configFilePath = configFilePath;

    return TRUE;
}

BOOL InitialiseConfig(HINSTANCE hInstance, HWND mainHWND)
{
    g_hInstance = hInstance;
    g_mainHWND = mainHWND;

    if (!GetConfigFilePath())
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CreateConfigDataFromRawConfig(RawConfigData *rawData, ConfigData *data)
{
    HFONT customTextListFont = CreateFontW(
        PointSizeToHeight(g_mainHWND, rawData->fontSize),
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


BOOL LoadConfigData(ConfigData *data)
{
    if (!PathFileExistsW(g_configFilePath))
    {
        if (!CreateConfigDataFromRawConfig(&g_defConfig, data))
        {
            return FALSE;
        }

        return TRUE;
    }

    IniFile iniFile;
    if (!ReadIniFile(g_configFilePath, &iniFile))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_FAILED_TO_READ_CONFIG_FILE_MSG,
            GetLastIniError()
        );

        return FALSE;
    }

    wchar_t *configCurrentlyReading = NULL;

    BOOL topMostEnabled;
    if (!GetBoolFromIni(&iniFile, GENERAL_SECTION, ALWAYS_ON_TOP_KEY, &topMostEnabled))
    {
        if (!SubstituteBoolDefault(&topMostEnabled, g_defConfig.topMostEnabled, TRUE))
        {
            configCurrentlyReading = GENERAL_SECTION L"." ALWAYS_ON_TOP_KEY;

            goto ShowErrorAndFail;
        }
    }

    wchar_t *fontName;
    if (!GetStringFromIni(&iniFile, FONT_SECTION, FONT_NAME_KEY, &fontName))
    {
        if (!SubstituteStringDefault(&fontName, g_defConfig.fontName, TRUE))
        {
            configCurrentlyReading = FONT_SECTION L"." FONT_NAME_KEY;

            goto ShowErrorAndFail;
        }
    }

    LONG fontSize;
    if (!GetLongFromIni(&iniFile, FONT_SECTION, FONT_SIZE_KEY, &fontSize))
    {
        if (!SubstituteLongDefault(&fontSize, g_defConfig.fontSize, TRUE))
        {
            configCurrentlyReading = FONT_SECTION L"." FONT_SIZE_KEY;

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
        IDS_INI_ERROR_READ_MSG,
        configCurrentlyReading,
        GetLastIniError()
    );

Fail:
    FreeIniFile(&iniFile);

    return FALSE;
}

BOOL SetTopMostEnabledConfig(BOOL enabled)
{
    if (!WriteIniValueBool(g_configFilePath, GENERAL_SECTION, ALWAYS_ON_TOP_KEY, enabled))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_INI_ERROR_WRITE_MSG,
            GENERAL_SECTION L"." ALWAYS_ON_TOP_KEY,
            GetLastIniError()
        );
        
        return FALSE;
    }

    return TRUE;
}

BOOL SetCustomTextListFontConfig(HFONT font)
{
    LOGFONTW lf = {0};
    GetObjectW(font, sizeof(LOGFONTW), &lf);

    wchar_t *fontName = lf.lfFaceName;

    if (!WriteIniValueString(g_configFilePath, FONT_SECTION, FONT_NAME_KEY, fontName))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_INI_ERROR_WRITE_MSG,
            FONT_SECTION L"." FONT_NAME_KEY,
            GetLastIniError()
        );

        return FALSE;
    }

    int fontSize = HeightToPointSize(g_mainHWND, lf.lfHeight);
    
    if (!WriteIniValueLong(g_configFilePath, FONT_SECTION, FONT_SIZE_KEY, fontSize))
    {
        MessageBoxUsingResourceString(
            g_hInstance,
            g_mainHWND,
            MB_ICONERROR | MB_OK,
            IDS_CONFIG_ERROR_TITLE,
            IDS_INI_ERROR_WRITE_MSG,
            FONT_SECTION L"." FONT_SIZE_KEY,
            GetLastIniError()
        );

        return FALSE;
    }

    return TRUE;
}

void FreeConfigFilePath(void)
{
    if (g_configFilePath != NULL)
    {
        free(g_configFilePath);
        g_configFilePath = NULL;
    }
}

